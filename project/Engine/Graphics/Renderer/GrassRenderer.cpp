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

void GrassRenderer::Initialize(const RenderEnvironment& env, const ModelData& grassModel)
{
    // 草のメッシュを初期化
    if (!grassModel.meshes.empty())
    {
        const auto& targetMesh = grassModel.meshes[0];
        mesh_.Initialize(env.device->GetDevice(), targetMesh.vertices, targetMesh.indices);
        mesh_.SetVertexCount(static_cast<uint32_t>(targetMesh.vertices.size()));
        mesh_.SetIndexCount(static_cast<uint32_t>(targetMesh.indices.size()));
    }

    UINT materialBufferSize = (sizeof(MaterialData) + 255) & ~255;

    // インスタンスバッファとマテリアルバッファをフレーム数分リングで確保
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
    // キューをクリアして、次のフレームのインデックスへ進める
    instanceQueue_.clear();
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void GrassRenderer::Submit(const Matrix4x4& world, const Vector4& color)
{
    // 最大数を超えたら追加しない（安全対策）
    if (instanceQueue_.size() >= kMaxInstances) return;

    GrassInstanceData data;
    data.world = world;
    data.color = color;

    instanceQueue_.push_back(data);
}

void GrassRenderer::Draw(const RenderEnvironment& env, uint32_t textureHandle, ShadowMap* shadowMap, const MaterialData& materialData)
{
    if (instanceQueue_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    uint32_t instanceCount = static_cast<uint32_t>(Math::MyMin((size_t)kMaxInstances, instanceQueue_.size()));

    // データのコピー 
    memcpy(mappedInstanceData_[currentFrameIndex_], instanceQueue_.data(), sizeof(GrassInstanceData) * instanceCount);
    memcpy(mappedMaterial_[currentFrameIndex_], &materialData, sizeof(MaterialData));

    // パイプライン設定
    cmdList->SetPipelineState(env.psoManager->GetPSO("Grass"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Grass"));

    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(2, materialResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(textureHandle));
    cmdList->SetGraphicsRootShaderResourceView(4, instanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress());

    if (shadowMap) 
    {
        cmdList->SetGraphicsRootDescriptorTable(5, shadowMap->GetSRVHandle());
    }

    // インスタンス描画実行
    cmdList->IASetVertexBuffers(0, 1, &mesh_.GetVertexBufferView());
    cmdList->IASetIndexBuffer(&mesh_.GetIndexBufferView());

    cmdList->DrawIndexedInstanced(
        static_cast<UINT>(mesh_.GetIndexCount()),
        instanceCount, 0, 0, 0);
}