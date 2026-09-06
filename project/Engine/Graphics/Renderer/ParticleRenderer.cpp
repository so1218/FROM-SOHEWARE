#include "pch.h"
#include "ParticleRenderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"
#include "PostEffectManager.h"
#include "PIXColors.h"

namespace FE
{

void ParticleRenderer::Initialize(const RenderEnvironment& env)
{
    // インスタンスバッファをフレーム数分リングで確保
    for (int i = 0; i < kFrameCount; ++i)
    {
        particleInstanceBuffer_[i] = BufferManager::CreateMappedBuffer(
            env.device->GetDevice(),
            kMaxCount,
            &mappedInstanceData_[i]
        );
    }

    // 最大数分確保して、毎フレームのメモリ割り当てを防ぐ
    requests_.reserve(kMaxCount);
}

void ParticleRenderer::BeginFrame()
{
    prevCount_ = static_cast<uint32_t>(requests_.size());
    requests_.clear();

    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void ParticleRenderer::Submit(const WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ, BlendMode blendMode, bool isBillboard, float intensity)
{
    if (requests_.size() >= kMaxCount) return;

    ParticleRequest req;

    req.data.position = { worldTransform.matWorld_.m[3][0], worldTransform.matWorld_.m[3][1], worldTransform.matWorld_.m[3][2] };

    req.data.scale.x = sqrtf(worldTransform.matWorld_.m[0][0] * worldTransform.matWorld_.m[0][0] + worldTransform.matWorld_.m[0][1] * worldTransform.matWorld_.m[0][1] + worldTransform.matWorld_.m[0][2] * worldTransform.matWorld_.m[0][2]);
    req.data.scale.y = sqrtf(worldTransform.matWorld_.m[1][0] * worldTransform.matWorld_.m[1][0] + worldTransform.matWorld_.m[1][1] * worldTransform.matWorld_.m[1][1] + worldTransform.matWorld_.m[1][2] * worldTransform.matWorld_.m[1][2]);

    req.data.color = Math::Uint32ToColorVector(color);
    req.data.textureIndex = textureIndex;
    req.data.rotationZ = rotationZ;
    req.data.isBillboard = isBillboard ? 1 : 0;
    req.data.intensity = intensity;
    req.blendMode = blendMode;
    req.textureIndex = textureIndex;

    requests_.push_back(req);
}

void ParticleRenderer::Draw(const RenderEnvironment& env)
{
    if (requests_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();

    PIXScopedEvent(cmdList, FE::PIXColors::Geometry, "Particle Pass (%zu Requests)", requests_.size());

    std::stable_sort(requests_.begin(), requests_.end(), [](const ParticleRequest& a, const ParticleRequest& b) {
        return a.blendMode < b.blendMode;
        });
    // インスタンスデータの一括コピー
    // 全パーティクルデータを一気にGPUバッファに送る
    ParticleInstanceData* dstBase = mappedInstanceData_[currentFrameIndex_];
    for (size_t i = 0; i < requests_.size(); ++i) {
        dstBase[i] = requests_[i].data;
    }

    // バッチ描画
    // 基本セットアップ
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Particle"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetGraphicsRootConstantBufferView(1, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(0));

    // インスタンスバッファの開始地点を取得
    D3D12_GPU_VIRTUAL_ADDRESS bufferGPUAddress = particleInstanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress();

    BlendMode lastBlendMode = static_cast<BlendMode>(-1);
    ID3D12PipelineState* currentPSO = nullptr;

    size_t drawCallStart = 0;
    while (drawCallStart < requests_.size()) {
        const auto& startReq = requests_[drawCallStart];

        // ブレンドモードが変わった時だけPSOをセット
        if (startReq.blendMode != lastBlendMode)
        {
            currentPSO = env.psoManager->GetPSO(GetPSOName(startReq.blendMode));
            if (currentPSO)
            {
                cmdList->SetPipelineState(currentPSO);
            }
            lastBlendMode = startReq.blendMode;
        }

        size_t drawCallEnd = drawCallStart + 1;
        while (drawCallEnd < requests_.size())
        {
            if (requests_[drawCallEnd].blendMode != startReq.blendMode)
            {
                break; // ブレンドモードが変わったので区切る
            }
            drawCallEnd++;
        }

        uint32_t instanceCount = static_cast<uint32_t>(drawCallEnd - drawCallStart);

        // インスタンスバッファの現在のオフセットをセット
        cmdList->SetGraphicsRootShaderResourceView(0, bufferGPUAddress + (sizeof(ParticleInstanceData) * drawCallStart));

        // 描画
        cmdList->DrawInstanced(6, instanceCount, 0, 0);

        drawCallStart = drawCallEnd;
    }
}

std::string ParticleRenderer::GetPSOName(BlendMode blendMode)
{
    switch (blendMode)
    {
    case kBlendModeNone:      return "ParticleOpaque";
    case kBlendModeAdd:       return "ParticleAdditive";
    case kBlendModeNormal:
    default:                  return "ParticleAlphaBlend";
    }
}

}