#include "pch.h"
#include "SpriteRenderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"
#include "PostEffectManager.h"
#include "Logger.h"
#include "PIXColors.h"

namespace FS
{


void SpriteRenderer::Initialize(const RenderEnvironment& env, int clientWidth, int clientHeight)
{
    ID3D12Device* device = env.device->GetDevice();

    // インスタンス用のバッファ (フレームごとに 1 つ)
    for (uint32_t frameIndex = 0; frameIndex < kFrameBufferCount; ++frameIndex)
    {
        instanceBuffers_[frameIndex] =
            BufferManager::CreateBufferResource(device, sizeof(SpriteInstance) * kMaxInstanceCount);
        instanceBuffers_[frameIndex]->Map(0, nullptr, reinterpret_cast<void**>(&mappedInstanceBuffers_[frameIndex]));
    }

    // 正射影行列 (左上が原点、右下が (画面幅, 画面高さ))。画面サイズは固定なので 1 回だけ作る
    frameConstantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(SpriteFrameConstants));
    frameConstantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedFrameConstants_));
    mappedFrameConstants_->projection = Matrix4x4::MakeOrthographic(
        0.0f, 0.0f, static_cast<float>(clientWidth), static_cast<float>(clientHeight), 0.0f, 100.0f);

    submittedSprites_.reserve(1024);
}

void SpriteRenderer::BeginFrame()
{
    submittedSprites_.clear();
}

void SpriteRenderer::Submit(const SpriteInstance& instance, int layerOrder)
{
    if (submittedSprites_.size() >= kMaxInstanceCount)
    {
        if (!hasWarnedOverflow_)
        {
            LOG_WARN("SpriteRenderer: 上限 ({}) を超えたため描画をスキップ", kMaxInstanceCount);
            hasWarnedOverflow_ = true;
        }
        return;
    }
    submittedSprites_.push_back({ instance, layerOrder });
}

void SpriteRenderer::Draw(const RenderEnvironment& env)
{
    lastDrawCount_ = static_cast<uint32_t>(submittedSprites_.size());
    if (lastDrawCount_ == 0)
    {
        return;
    }

    // 重なり順で並べる。同じ値なら Submit した順 (stable_sort なので順番が保たれる)
    std::stable_sort(submittedSprites_.begin(), submittedSprites_.end(),
        [](const SubmittedSprite& a, const SubmittedSprite& b)
        {
            return a.layerOrder < b.layerOrder;
        });

    // 並べた順にバッファへコピー。GPU はインスタンスの順番どおりに重ねて描く
    SpriteInstance* instanceDestination = mappedInstanceBuffers_[currentFrameIndex_];
    for (size_t i = 0; i < submittedSprites_.size(); ++i)
    {
        instanceDestination[i] = submittedSprites_[i].instance;
    }

    ID3D12GraphicsCommandList* commandList = env.commandManager->GetCommandList();
    PIXScopedEvent(commandList, FS::PIXColors::UI, "Sprite Pass (%u Sprites)", lastDrawCount_);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetPipelineState(env.psoManager->GetPSO("Sprite"));
    commandList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Sprite"));

    commandList->SetGraphicsRootConstantBufferView(
        0, frameConstantBuffer_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootShaderResourceView(
        1, instanceBuffers_[currentFrameIndex_]->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(
        2, env.srvManager->GetSRVHandleGPU(0));  

    // 6 頂点 × 枚数 を 1 回で描画
    commandList->DrawInstanced(6, lastDrawCount_, 0, 0);

    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameBufferCount;
}

}