#include "pch.h"
#include "TreeRenderer.h"
#include "BufferManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "CommandManager.h"
#include "GraphicsDevice.h"
#include "EnvironmentManager.h"

namespace FE
{

void TreeRenderer::Initialize(const RenderEnvironment& env)
{
    device_ = env.device;

    // インスタンシング用 StructuredBuffer の作成
    instanceBuffer_.resource = BufferManager::CreateMappedBuffer(
        device_->GetDevice(),
        kMaxInstances,
        &instanceBuffer_.mapped
    );

    // SRV（StructuredBuffer）の作成
    instanceBuffer_.srvIndex = env.srvManager->Allocate();
    instanceBuffer_.srvIndex = env.srvManager->CreateStructuredBufferSRV(
        instanceBuffer_.resource.Get(),
        kMaxInstances,
        sizeof(TreeInstanceData)
    );
}

void TreeRenderer::BeginFrame()
{
    submissions_.clear();
    batches_.clear();
    currentInstanceLocation_ = 0;
}

const std::vector<Mesh>& TreeRenderer::GetOrCreateBatch(const ModelData& modelData)
{
    auto it = meshCache_.find(&modelData);
    if (it != meshCache_.end()) return it->second.meshes;

    ModelBatch batch;
    batch.meshes.resize(modelData.meshes.size());

    for (size_t i = 0; i < modelData.meshes.size(); ++i)
    {
        batch.meshes[i].Initialize(device_->GetDevice(), modelData.meshes[i].vertices, modelData.meshes[i].indices);
        batch.meshes[i].SetVertexCount(static_cast<uint32_t>(modelData.meshes[i].vertices.size()));
        batch.meshes[i].SetIndexCount(static_cast<uint32_t>(modelData.meshes[i].indices.size()));
    }

    meshCache_[&modelData] = std::move(batch);
    return meshCache_[&modelData].meshes;
}

void TreeRenderer::Submit(
    const WorldTransform& worldTransform,
    const ModelData& modelData,
    const TreeMaterialHandle& treeMaterial,
    const Vector4& colorVariation,
    float lodFade)
{
    // メッシュバッチの登録・キャッシュ
    GetOrCreateBatch(modelData);

    std::function<void(const Node&, const Matrix4x4&)> Traverse =
        [&](const Node& node, const Matrix4x4& parentMatrix)
        {
            Matrix4x4 currentWorld = node.localMatrix * parentMatrix;

            for (unsigned int meshIndex : node.meshIndices)
            {
                if (submissions_.size() >= kMaxInstances) return;

                TreeSubmission sub{};
                sub.modelData = &modelData;
                sub.meshIndex = meshIndex;
                sub.treeMaterial = treeMaterial; 
                sub.worldMatrix = currentWorld;
                sub.colorVariation = colorVariation;
                sub.lodFade = lodFade;

                // ★判定：メッシュIndex 0 を「幹」、1以降を「葉」として判定
                sub.isLeaf = (meshIndex >= 1);

                submissions_.push_back(sub);
            }

            for (const auto& child : node.children)
            {
                Traverse(child, currentWorld);
            }
        };

    Traverse(modelData.rootNode, worldTransform.matWorld_);
}

void TreeRenderer::PrepareBatches()
{
    batches_.clear();
    if (submissions_.empty()) return;

    // ソート条件の更新
    std::sort(submissions_.begin(), submissions_.end(),
        [](const TreeSubmission& a, const TreeSubmission& b) {
            if (a.isLeaf != b.isLeaf) return a.isLeaf < b.isLeaf;
            if (a.modelData != b.modelData) return a.modelData < b.modelData;
            if (a.meshIndex != b.meshIndex) return a.meshIndex < b.meshIndex;
            // 定数バッファのポインタでマテリアルの違いを判定
            if (a.treeMaterial.leafMaterialBuffer.Get() != b.treeMaterial.leafMaterialBuffer.Get())
                return a.treeMaterial.leafMaterialBuffer.Get() < b.treeMaterial.leafMaterialBuffer.Get();
            if (a.treeMaterial.trunkMaterialBuffer.Get() != b.treeMaterial.trunkMaterialBuffer.Get())
                return a.treeMaterial.trunkMaterialBuffer.Get() < b.treeMaterial.trunkMaterialBuffer.Get();
            return false;
        });

    uint32_t instanceCount = 0;
    currentInstanceLocation_ = 0;

    for (size_t i = 0; i < submissions_.size(); ++i)
    {
        const auto& sub = submissions_[i];
        auto& instanceGPU = instanceBuffer_.mapped[currentInstanceLocation_ + instanceCount];
        instanceGPU.worldMatrix = sub.worldMatrix;
        instanceGPU.colorVariation = sub.colorVariation;
        instanceGPU.lodFade = sub.lodFade;

        instanceCount++;
        bool isLast = (i == submissions_.size() - 1);
        bool shouldFlush = isLast;

        if (!isLast)
        {
            const auto& nextSub = submissions_[i + 1];
            // バッチ分割条件の更新
            if (sub.isLeaf != nextSub.isLeaf ||
                sub.modelData != nextSub.modelData ||
                sub.meshIndex != nextSub.meshIndex ||
                sub.treeMaterial.leafMaterialBuffer.Get() != nextSub.treeMaterial.leafMaterialBuffer.Get() ||
                sub.treeMaterial.trunkMaterialBuffer.Get() != nextSub.treeMaterial.trunkMaterialBuffer.Get())
            {
                shouldFlush = true;
            }
        }

        if (shouldFlush)
        {
            TreeBatch batch{};
            batch.modelData = sub.modelData;
            batch.meshIndex = sub.meshIndex;
            batch.treeMaterial = sub.treeMaterial; 
            batch.isLeaf = sub.isLeaf;
            batch.instanceCount = instanceCount;
            batch.startInstanceLocation = currentInstanceLocation_;

            batches_.push_back(batch);

            currentInstanceLocation_ += instanceCount;
            instanceCount = 0;
            if (currentInstanceLocation_ >= kMaxInstances) break;
        }
    }
}

void TreeRenderer::Draw(const RenderEnvironment& env, ShadowMap* shadowMap, uint32_t windMapSrvIndex)
{
    if (batches_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (const auto& batch : batches_)
    {
        const auto& meshes = GetOrCreateBatch(*batch.modelData);
        const Mesh* mesh = &meshes[batch.meshIndex];
        uint32_t indexCount = static_cast<uint32_t>(mesh->GetIndexCount());

        if (batch.isLeaf)
        {
            cmdList->SetPipelineState(env.psoManager->GetPSO("TreeFoliage"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("TreeFoliage"));

            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(2, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(3, batch.treeMaterial.leafMaterialBuffer->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());

            cmdList->SetGraphicsRoot32BitConstant(5, batch.startInstanceLocation, 0);

            cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.envMapHandle));
            cmdList->SetGraphicsRootDescriptorTable(7, shadowMap->GetSRVHandle());
            cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));
            cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));
            // ★葉のテクスチャ
            cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.leafTextureHandle));
            cmdList->SetGraphicsRootDescriptorTable(11, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.leafNormalMapHandle));
            cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.leafMetallicRoughnessHandle));
        }
        else
        {
            cmdList->SetPipelineState(env.psoManager->GetPSO("TreeTrunk"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("TreeTrunk"));

            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetPointLightResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetSpotLightResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(4, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());

            cmdList->SetGraphicsRootConstantBufferView(5, batch.treeMaterial.leafMaterialBuffer->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(6, batch.treeMaterial.trunkMaterialBuffer->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(7, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());

            cmdList->SetGraphicsRoot32BitConstant(8, batch.startInstanceLocation, 0);

            cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.trunkTextureHandle));
            cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.envMapHandle));
            cmdList->SetGraphicsRootDescriptorTable(11, shadowMap->GetSRVHandle());
            cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.toonRampHandle));
            cmdList->SetGraphicsRootDescriptorTable(13, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.trunkNormalMapHandle));
            cmdList->SetGraphicsRootDescriptorTable(14, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));
            cmdList->SetGraphicsRootDescriptorTable(15, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));
        }

        cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        cmdList->DrawIndexedInstanced(indexCount, batch.instanceCount, 0, 0, 0);
    }
}

void TreeRenderer::DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex, uint32_t windMapSrvIndex)
{
    if (batches_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (const auto& batch : batches_)
    {
        const auto& meshes = GetOrCreateBatch(*batch.modelData);
        const Mesh* mesh = &meshes[batch.meshIndex];
        uint32_t indexCount = static_cast<uint32_t>(mesh->GetIndexCount());

        if (batch.isLeaf)
        {
            cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapTreeFoliage"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapTreeFoliage"));

            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(1, batch.treeMaterial.leafMaterialBuffer->GetGPUVirtualAddress());
            cmdList->SetGraphicsRoot32BitConstant(2, batch.startInstanceLocation, 0);
            cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRoot32BitConstant(4, cascadeIndex, 0);
            cmdList->SetGraphicsRootConstantBufferView(5, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());

            cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));
            cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));
            cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.leafTextureHandle));
        }
        else
        {
            cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapTreeTrunk"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapTreeTrunk"));

            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(1, batch.treeMaterial.leafMaterialBuffer->GetGPUVirtualAddress());
            cmdList->SetGraphicsRoot32BitConstant(2, batch.startInstanceLocation, 0);
            cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRoot32BitConstant(4, cascadeIndex, 0);
            cmdList->SetGraphicsRootConstantBufferView(5, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());

            cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));
            cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));
        }

        cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        cmdList->DrawIndexedInstanced(indexCount, batch.instanceCount, 0, 0, 0);
    }
}

}