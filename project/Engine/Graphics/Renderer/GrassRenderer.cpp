#include "pch.h"
#include "GrassRenderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"
#include "PostEffectManager.h"

namespace FE
{

void GrassRenderer::Initialize(const RenderEnvironment& env)
{
    // メッシュ初期化は不要（頂点シェーダーで生成するため）
    UINT materialBufferSize = (sizeof(GrassMaterialData) + 255) & ~255;

    for (int i = 0; i < kFrameCount; ++i)
    {
        // インスタンスバッファ 
        instanceBuffer_[i] = BufferManager::CreateBufferResource(
            env.device->GetDevice(),
            sizeof(GrassInstanceData) * kMaxInstances);

        instanceBuffer_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedInstanceData_[i]));

        // マテリアルバッファ 
        materialResource_[i] = BufferManager::CreateBufferResource(
            env.device->GetDevice(),
            materialBufferSize);

        materialResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedMaterial_[i]));
    }
}

void GrassRenderer::BeginFrame()
{
    instanceQueue_.clear();
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void GrassRenderer::Submit(const Vector3& position, float height, float rotationY, float width, uint32_t packedColor)
{
    if (instanceQueue_.size() >= kMaxInstances) return;

    GrassInstanceData data;
    data.posAndHeight = Vector4(position.x, position.y, position.z, height);

    // uintのカラービットパターンをそのままfloatに再解釈して渡す
    float colorAsFloat;
    std::memcpy(&colorAsFloat, &packedColor, sizeof(float));

    data.rotWidthColor = Vector4(rotationY, width, colorAsFloat, 0.0f);

    instanceQueue_.push_back(data);
}

void GrassRenderer::Draw(const RenderEnvironment& env, uint32_t windMapTextureHandle, ShadowMap* shadowMap, const GrassMaterialData& materialData)
{
    if (instanceQueue_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    uint32_t instanceCount = static_cast<uint32_t>(Math::MyMin((size_t)kMaxInstances, instanceQueue_.size()));

    // 定数バッファ・インスタンスバッファへ転送
    memcpy(mappedInstanceData_[currentFrameIndex_], instanceQueue_.data(), sizeof(GrassInstanceData) * instanceCount);
    memcpy(mappedMaterial_[currentFrameIndex_], &materialData, sizeof(GrassMaterialData));

    // パイプライン・ルートシグネチャ設定
    cmdList->SetPipelineState(env.psoManager->GetPSO("Grass"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Grass"));

    // プロシージャル生成(7頂点)のため Triangle Strip を使用
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Root Parameters の設定
    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(2, materialResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootShaderResourceView(4, instanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(5, env.srvManager->GetSRVHandleGPU(windMapTextureHandle));

    if (shadowMap)
    {
        cmdList->SetGraphicsRootDescriptorTable(6, shadowMap->GetSRVHandle());
    }

    cmdList->DrawInstanced(7, instanceCount, 0, 0);
}

}