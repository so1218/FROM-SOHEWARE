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
    const std::vector<MaterialHandle>& materials,
    const Vector4& colorVariation,
    float lodFade)
{
    // メッシュバッチの登録・キャッシュ
    GetOrCreateBatch(modelData);

    // ノード階層を巡回
    std::function<void(const Node&, const Matrix4x4&)> Traverse =
        [&](const Node& node, const Matrix4x4& parentMatrix)
        {
            Matrix4x4 currentWorld = node.localMatrix * parentMatrix;

            for (unsigned int meshIndex : node.meshIndices)
            {
                if (submissions_.size() >= kMaxInstances) return;

                const auto& meshPart = modelData.meshes[meshIndex];

                MaterialHandle actualMaterial;
                if (meshIndex < materials.size())
                {
                    actualMaterial = materials[meshIndex];
                }
                else
                {
                    actualMaterial = materials.empty() ? meshPart.materialHandle : materials[0];
                }

                TreeSubmission sub{};
                sub.modelData = &modelData;
                sub.meshIndex = meshIndex;
                sub.materialHandle = actualMaterial;
                sub.worldMatrix = currentWorld;
                sub.colorVariation = colorVariation;
                sub.lodFade = lodFade;

                // ★判定：マテリアル/メッシュIndex 0 を「葉」、1以降を「幹」として判定
                // （マテリアルデータ自体に isLeaf フラグや名前識別がある場合はそちらを利用）
                sub.isLeaf = (meshIndex == 0);

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

    // バッチ最適化のためのソート
    std::sort(submissions_.begin(), submissions_.end(),
        [](const TreeSubmission& a, const TreeSubmission& b) {
            if (a.isLeaf != b.isLeaf) return a.isLeaf < b.isLeaf;
            if (a.modelData != b.modelData) return a.modelData < b.modelData;
            if (a.meshIndex != b.meshIndex) return a.meshIndex < b.meshIndex;
            return a.materialHandle.materialData < b.materialHandle.materialData;
        });

    uint32_t instanceCount = 0;
    currentInstanceLocation_ = 0;

    for (size_t i = 0; i < submissions_.size(); ++i)
    {
        const auto& sub = submissions_[i];

        // StructuredBuffer へ転送するインスタンスデータ
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
            // 描画ステート・モデル・メッシュ・マテリアル・葉/幹の切り替わりでバッチを分割
            if (sub.isLeaf != nextSub.isLeaf ||
                sub.modelData != nextSub.modelData ||
                sub.meshIndex != nextSub.meshIndex ||
                sub.materialHandle.materialData != nextSub.materialHandle.materialData)
            {
                shouldFlush = true;
            }
        }

        if (shouldFlush)
        {
            TreeBatch batch{};
            batch.modelData = sub.modelData;
            batch.meshIndex = sub.meshIndex;
            batch.materialHandle = sub.materialHandle;
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
        assert(batch.meshIndex < meshes.size());
        const Mesh* mesh = &meshes[batch.meshIndex];
        uint32_t indexCount = static_cast<uint32_t>(mesh->GetIndexCount());

        if (batch.isLeaf)
        {
            // -----------------------------------------------------------------
            // 葉（Foliage）描画パス
            // -----------------------------------------------------------------
            cmdList->SetPipelineState(env.psoManager->GetPSO("Foliage"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Foliage"));

            // ConstantBuffers
            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress()); // b0: FrameData
            cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress()); // b1: Directional Light
            cmdList->SetGraphicsRootConstantBufferView(5, batch.materialHandle.resource->GetGPUVirtualAddress()); // b5: LeafMaterialData
            cmdList->SetGraphicsRootConstantBufferView(8, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress()); // b8: ShadowData

            // SRVs (すべて MaterialHandle と 引数 からスマートに取得)
            cmdList->SetGraphicsRootDescriptorTable(2, shadowMap->GetSRVHandle());                                                  // t2: Cascade Shadow Map
            cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));                 // t10: TreeInstanceData
            cmdList->SetGraphicsRootDescriptorTable(11, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));                          // t11: 引数の風マップ
            cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(batch.materialHandle.textureHandle));        // t12: Albedo / Alpha
            cmdList->SetGraphicsRootDescriptorTable(13, env.srvManager->GetSRVHandleGPU(batch.materialHandle.normalMapHandle));      // t13: Normal Map
            cmdList->SetGraphicsRootDescriptorTable(14, env.srvManager->GetSRVHandleGPU(batch.materialHandle.metallicRoughnessHandle));// t14: MetallicRoughness

            cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

            cmdList->DrawIndexedInstanced(indexCount, batch.instanceCount, 0, 0, batch.startInstanceLocation);
        }
        else
        {
            // -----------------------------------------------------------------
            // 幹（Trunk）描画パス（通常モデルのインスタンシング描画）
            // -----------------------------------------------------------------
            cmdList->SetPipelineState(env.psoManager->GetPSO("Object3D_Opaque"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Instancing3D"));

            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetPointLightResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetSpotLightResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetAreaLightResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(5, batch.materialHandle.resource->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(7, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());

            cmdList->SetGraphicsRoot32BitConstant(6, batch.startInstanceLocation, 0);
            cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(batch.materialHandle.textureHandle));
            cmdList->SetGraphicsRootDescriptorTable(10, shadowMap->GetSRVHandle());
            cmdList->SetGraphicsRootDescriptorTable(17, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));

            cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

            cmdList->DrawIndexedInstanced(indexCount, batch.instanceCount, 0, 0, batch.startInstanceLocation);
        }
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
        assert(batch.meshIndex < meshes.size());
        const Mesh* mesh = &meshes[batch.meshIndex];
        uint32_t indexCount = static_cast<uint32_t>(mesh->GetIndexCount());

        if (batch.isLeaf)
        {
            // =================================================================
            // 【葉（Leaf）の影】
            // 風の揺れ + アルファテスト（葉の打ち抜き）が必要なため専用PSOを使用
            // =================================================================
            cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapFoliage"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapFoliage"));

            // 定数バッファ
            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(2, batch.materialHandle.resource->GetGPUVirtualAddress()); // LeafMaterialData
            cmdList->SetGraphicsRoot32BitConstant(3, batch.startInstanceLocation, 0);
            cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRoot32BitConstant(5, cascadeIndex, 0);

            // テクスチャ / SRV
            cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));
            cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));                       // 風マップ
            cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(batch.materialHandle.textureHandle));   // アルファ抜き用テクスチャ

            cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

            cmdList->DrawIndexedInstanced(indexCount, batch.instanceCount, 0, 0, batch.startInstanceLocation);
        }
        else
        {
            // =================================================================
            // 【幹（Trunk）の影】
            // ModelRenderer の「静的モデル・通常影」と全く同じ PSO / RootSignature を流用！
            // =================================================================
            cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapInstanced"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapInstanced"));

            // ModelRenderer と完全に一致させる
            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(2, batch.materialHandle.resource->GetGPUVirtualAddress());
            cmdList->SetGraphicsRoot32BitConstant(3, batch.startInstanceLocation, 0);
            cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRoot32BitConstant(5, cascadeIndex, 0);
            cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));

            cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

            cmdList->DrawIndexedInstanced(indexCount, batch.instanceCount, 0, 0, batch.startInstanceLocation);
        }
    }
}

}