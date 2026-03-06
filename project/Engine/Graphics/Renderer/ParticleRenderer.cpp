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

void ParticleRenderer::Initialize(const RenderEnvironment& env)
{
    // インスタンスバッファをフレーム数分リングで確保
    for (int i = 0; i < kFrameCount; ++i)
    {
        particleInstanceBuffer_[i] = BufferManager::CreateBufferResource(
            env.device->GetDevice(),
            sizeof(ParticleInstanceData) * kMaxCount);

        particleInstanceBuffer_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedInstanceData_[i]));
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

    std::sort(requests_.begin(), requests_.end());
    // インスタンスデータの一括コピー
    // 全パーティクルデータを一気にGPUバッファに送る
    ParticleInstanceData* dstBase = mappedInstanceData_[currentFrameIndex_];
    for (size_t i = 0; i < requests_.size(); ++i) {
        dstBase[i] = requests_[i].data;
    }

    // バッチ描画
    auto* cmdList = env.commandManager->GetCommandList();
    // 基本セットアップ
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Particle"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetGraphicsRootConstantBufferView(1, env.globalConstants->GetResource()->GetGPUVirtualAddress());

    // インスタンスバッファの開始地点を取得
    D3D12_GPU_VIRTUAL_ADDRESS bufferGPUAddress = particleInstanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress();

    BlendMode lastBlendMode = static_cast<BlendMode>(-1); // 初期化
    ID3D12PipelineState* currentPSO = nullptr;

    size_t drawCallStart = 0; // 描画開始インデックス
    while (drawCallStart < requests_.size()) {
        const auto& startReq = requests_[drawCallStart];

        // ブレンドモードが変わった時だけPSOを再取得してセット
        if (startReq.blendMode != lastBlendMode) 
        {
            currentPSO = env.psoManager->GetPSO(GetPSOName(startReq.blendMode));
            if (currentPSO)
            {
                cmdList->SetPipelineState(currentPSO);
            }
            lastBlendMode = startReq.blendMode;
        }
        
        // 同じ設定がどこまで続くか探す
        size_t drawCallEnd = drawCallStart + 1;
        while (drawCallEnd < requests_.size()) 
        {
            if (requests_[drawCallEnd].blendMode != startReq.blendMode || 
                requests_[drawCallEnd].textureIndex != startReq.textureIndex) 
            {
                break; // 設定が変わったので区切る
            }
            drawCallEnd++;
        }

        // 1回描画
        uint32_t instanceCount = static_cast<uint32_t>(drawCallEnd - drawCallStart);

        // テクスチャセット
        cmdList->SetGraphicsRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(startReq.textureIndex));

        // インスタンスバッファの現在のオフセットをセット
        cmdList->SetGraphicsRootShaderResourceView(0, bufferGPUAddress + (sizeof(ParticleInstanceData) * drawCallStart));

        // 描画
        cmdList->DrawInstanced(6, instanceCount, 0, 0);

        // 次のバッチへ
        drawCallStart = drawCallEnd;
    }
}

std::string ParticleRenderer::GetPSOName(BlendMode blendMode)
{
    switch (blendMode)
    {
    case kBlendModeNone:      return "ParticleOpaque";
    case kBlendModeAdd:       return "ParticleAdditive";
    case kBlendModeSubtract:  return "ParticleSubtract";
    case kBlendModeMultiply:  return "ParticleMultiply";
    case kBlendModeScreen:    return "ParticleScreen";
    case kBlendModeExclusion: return "ParticleExclusion";
    case kBlendModeNormal:
    default:                  return "ParticleAlphaBlend";
    }
}