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
    // 頂点・インデックス(1枚の板ポリ)
    std::vector<VertexData> vertices = {
        {{-0.5f, -0.5f, 0, 1}, {0, 1}, {0, 0, -1}},
        {{ 0.5f, -0.5f, 0, 1}, {1, 1}, {0, 0, -1}},
        {{-0.5f,  0.5f, 0, 1}, {0, 0}, {0, 0, -1}},
        {{ 0.5f,  0.5f, 0, 1}, {1, 0}, {0, 0, -1}},
    };
    std::vector<uint32_t> indices = { 0, 1, 2, 1, 3, 2 };

    mesh_.Initialize(env.device->GetDevice(), vertices, indices);

    // インスタンスバッファをフレーム数分リングで確保
    for (int i = 0; i < kFrameCount; ++i)
    {
        particleInstanceBuffer_[i] = BufferManager::CreateBufferResource(
            env.device->GetDevice(),
            sizeof(ParticleInstanceData) * kMaxCount);

        particleInstanceBuffer_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedInstanceData_[i]));
    }
}

void ParticleRenderer::BeginFrame()
{
    prevCount_ = index_;
    index_ = 0;

    for (auto& blendPair : batches_)
    {
        for (auto& texPair : blendPair.second)
        {
            texPair.second.clear(); // std::vectorの中身だけを空に
        }
    }

    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void ParticleRenderer::Submit(const WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ, BlendMode blendMode, bool isBillboard, float intensity)
{
    if (index_ >= kMaxCount) return;

    ParticleInstanceData data;

    data.position = { worldTransform.matWorld_.m[3][0], worldTransform.matWorld_.m[3][1], worldTransform.matWorld_.m[3][2] };

    data.scale.x = sqrtf(worldTransform.matWorld_.m[0][0] * worldTransform.matWorld_.m[0][0] + worldTransform.matWorld_.m[0][1] * worldTransform.matWorld_.m[0][1] + worldTransform.matWorld_.m[0][2] * worldTransform.matWorld_.m[0][2]);
    data.scale.y = sqrtf(worldTransform.matWorld_.m[1][0] * worldTransform.matWorld_.m[1][0] + worldTransform.matWorld_.m[1][1] * worldTransform.matWorld_.m[1][1] + worldTransform.matWorld_.m[1][2] * worldTransform.matWorld_.m[1][2]);

    data.color = Math::Uint32ToColorVector(color);
    data.textureIndex = textureIndex;
    data.rotationZ = rotationZ;
    data.isBillboard = isBillboard ? 1 : 0;
    data.intensity = intensity;

    batches_[blendMode][textureIndex].push_back(data);
    index_++;
}

void ParticleRenderer::Draw(const RenderEnvironment& env)
{
    if (index_ == 0) return;

    auto* cmdList = env.commandManager->GetCommandList();

    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Particle"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetIndexBuffer(&mesh_.GetIndexBufferView());
    cmdList->IASetVertexBuffers(0, 1, &mesh_.GetVertexBufferView());

    cmdList->SetGraphicsRootConstantBufferView(1, env.globalConstants->GetResource()->GetGPUVirtualAddress());

    ParticleInstanceData* dstBase = mappedInstanceData_[currentFrameIndex_];
    size_t currentOffset = 0;

    for (auto& [blendMode, textureMap] : batches_)
    {
        std::string psoName;
        switch (blendMode)
        {
        case kBlendModeNone:      psoName = "ParticleOpaque"; break;
        case kBlendModeAdd:       psoName = "ParticleAdditive"; break;
        case kBlendModeSubtract:  psoName = "ParticleSubtract"; break;
        case kBlendModeMultiply:  psoName = "ParticleMultiply"; break;
        case kBlendModeScreen:    psoName = "ParticleScreen"; break;
        case kBlendModeExclusion: psoName = "ParticleExclusion"; break;
        case kBlendModeNormal:
        default:                  psoName = "ParticleAlphaBlend"; break;
        }

        ID3D12PipelineState* pso = env.psoManager->GetPSO(psoName);
        if (!pso) continue;
        cmdList->SetPipelineState(pso);

        for (auto& [textureIndex, instances] : textureMap)
        {
            if (instances.empty()) continue;

            // CPUからGPUへコピー
            ParticleInstanceData* dst = dstBase + currentOffset;
            memcpy(dst, instances.data(), sizeof(ParticleInstanceData) * instances.size());

            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = env.srvManager->GetSRVHandleGPU(textureIndex);
            cmdList->SetGraphicsRootDescriptorTable(3, srvHandle);

            UINT64 gpuAddress = particleInstanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress();
            gpuAddress += sizeof(ParticleInstanceData) * currentOffset;
            cmdList->SetGraphicsRootShaderResourceView(0, gpuAddress);

            cmdList->DrawIndexedInstanced(
                static_cast<UINT>(mesh_.GetIndexCount()),
                static_cast<UINT>(instances.size()), 0, 0, 0);

            currentOffset += instances.size();
        }
    }
}