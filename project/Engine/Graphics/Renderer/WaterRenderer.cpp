#include "pch.h"
#include "WaterRenderer.h"
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

void WaterRenderer::Initialize(const RenderEnvironment& env)
{
    device_ = env.device;

    // インスタンシング用 StructuredBuffer の作成
    instanceBuffer_.resource = BufferManager::CreateMappedBuffer(
        device_->GetDevice(),
        kMaxInstances,
        &instanceBuffer_.mapped
    );

    // SRV の作成（CreateStructuredBufferSRV 内部で Allocate と SRV 生成を行うため 1 行で完結）
    instanceBuffer_.srvIndex = env.srvManager->CreateStructuredBufferSRV(
        instanceBuffer_.resource.Get(),
        kMaxInstances,
        sizeof(Object3DInstanceData)
    );
}

void WaterRenderer::Finalize()
{
    meshCache_.clear();
}

void WaterRenderer::BeginFrame()
{
    waterSubmissions_.clear();
    batches_.clear();
    currentInstanceLocation_ = 0;
}

void WaterRenderer::SetCameraState(const Matrix4x4& view, const Matrix4x4& viewProjection)
{
    viewMatrix_ = view;
    viewProjectionMatrix_ = viewProjection;
}

void WaterRenderer::SetSceneTextures(
    D3D12_GPU_DESCRIPTOR_HANDLE sceneColorSRV,
    D3D12_GPU_DESCRIPTOR_HANDLE sceneDepthSRV)
{
    sceneColorSRV_ = sceneColorSRV;
    sceneDepthSRV_ = sceneDepthSRV;
}

const std::vector<Mesh>& WaterRenderer::GetOrCreateBatch(const ModelData& modelData)
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

void WaterRenderer::Submit(
    const WorldTransform& worldTransform,
    const ModelData& modelData,
    D3D12_GPU_VIRTUAL_ADDRESS waterMaterialCBV,
    uint32_t normalMapHandle,
    uint32_t rippleTextureHandle,
    uint32_t envMapSrvHandle,
    const Vector4& instanceColor)
{
    GetOrCreateBatch(modelData);

    std::function<void(const Node&, const Matrix4x4&, const Matrix4x4&)> Traverse =
        [&](const Node& node, const Matrix4x4& parentMatrix, const Matrix4x4& parentPrevMatrix)
        {
            Matrix4x4 currentWorldMatrix = node.localMatrix * parentMatrix;
            Matrix4x4 currentPrevWorldMatrix = node.localMatrix * parentPrevMatrix;

            for (unsigned int meshIndex : node.meshIndices)
            {
                Matrix4x4 wvp = currentWorldMatrix * viewProjectionMatrix_;
                Matrix4x4 worldView = currentWorldMatrix * viewMatrix_;

                WaterSubmission sub{};
                sub.modelData = &modelData;
                sub.meshIndex = meshIndex;
                sub.worldMatrix = currentWorldMatrix;
                sub.worldInverseTranspose = Matrix4x4::Inverse(currentWorldMatrix.Transpose());
                sub.wvpMatrix = wvp;
                sub.prevWorldMatrix = currentPrevWorldMatrix;
                sub.instanceColor = instanceColor;

                sub.waterMaterialCBV = waterMaterialCBV;
                sub.normalMapHandle = normalMapHandle;
                sub.rippleTextureHandle = rippleTextureHandle;
                sub.envMapSrvHandle = envMapSrvHandle;

                sub.depth = worldView.m[3][2]; // 奥から手前へのソート用 Z 値

                waterSubmissions_.push_back(sub);
            }

            for (const auto& child : node.children)
            {
                Traverse(child, currentWorldMatrix, currentPrevWorldMatrix);
            }
        };

    Traverse(modelData.rootNode, worldTransform.matWorld_, worldTransform.matWorldPrev_);
}

void WaterRenderer::PrepareBatches()
{
    batches_.clear();
    if (waterSubmissions_.empty()) return;

    // 水面同士の前後関係を正しく描画するため、奥(depth大)から手前(depth小)へソート
    std::sort(waterSubmissions_.begin(), waterSubmissions_.end(),
        [](const WaterSubmission& a, const WaterSubmission& b) {
            if (a.depth != b.depth) return a.depth > b.depth;
            if (a.modelData != b.modelData) return a.modelData < b.modelData;
            if (a.meshIndex != b.meshIndex) return a.meshIndex < b.meshIndex;
            return a.waterMaterialCBV < b.waterMaterialCBV;
        });

    uint32_t instanceCount = 0;

    for (size_t i = 0; i < waterSubmissions_.size(); ++i)
    {
        const auto& sub = waterSubmissions_[i];

        // インスタンスデータをバッファへ設定
        auto& instanceData = instanceBuffer_.mapped[currentInstanceLocation_ + instanceCount];
        instanceData.World = sub.worldMatrix;
        instanceData.WorldInverseTranspose = sub.worldInverseTranspose;
        instanceData.WorldColor = sub.instanceColor;
        instanceData.PrevWorld = sub.prevWorldMatrix;

        instanceCount++;

        bool isLast = (i == waterSubmissions_.size() - 1);
        bool shouldFlush = isLast;

        if (!isLast)
        {
            const auto& nextSub = waterSubmissions_[i + 1];
            // 同一メッシュかつ同一マテリアルバッファの場合のみインスタンシング
            if (sub.modelData != nextSub.modelData ||
                sub.meshIndex != nextSub.meshIndex ||
                sub.waterMaterialCBV != nextSub.waterMaterialCBV ||
                sub.normalMapHandle != nextSub.normalMapHandle ||
                sub.rippleTextureHandle != nextSub.rippleTextureHandle ||
                sub.envMapSrvHandle != nextSub.envMapSrvHandle)
            {
                shouldFlush = true;
            }
        }

        if (shouldFlush)
        {
            WaterBatch batch;
            batch.baseSubmission = &sub;
            batch.instanceCount = instanceCount;
            batch.startInstanceLocation = currentInstanceLocation_;
            batches_.push_back(batch);

            currentInstanceLocation_ += instanceCount;
            instanceCount = 0;

            if (currentInstanceLocation_ >= kMaxInstances) break;
        }
    }
}

void WaterRenderer::Draw(const RenderEnvironment& env)
{
    if (batches_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // RootSignature のバインド
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Water"));

    // フレーム共通バッファのセット (b0〜b3)
    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress()); // b0
    cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress()); // b1
    cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetPointLightResource()->GetGPUVirtualAddress()); // b2
    cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetSpotLightResource()->GetGPUVirtualAddress()); // b3

    // シーンテクスチャ・環境マップ (t0〜t2)
    cmdList->SetGraphicsRootDescriptorTable(6, sceneColorSRV_); // t0
    cmdList->SetGraphicsRootDescriptorTable(7, sceneDepthSRV_); // t1

    // インスタンス構造化バッファ (t10)
    cmdList->SetGraphicsRootDescriptorTable(11, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex)); // t10

    for (const auto& batch : batches_)
    {
        const auto& sub = *batch.baseSubmission;
        const auto& meshes = GetOrCreateBatch(*sub.modelData);
        const auto& mesh = meshes[sub.meshIndex];

        // PSO
        cmdList->SetPipelineState(env.psoManager->GetPSO("Water"));

        // バッチ個別パラメータ
        cmdList->SetGraphicsRootConstantBufferView(4, sub.waterMaterialCBV); // b5
        cmdList->SetGraphicsRoot32BitConstant(5, batch.startInstanceLocation, 0); // b7 (InstanceOffset)

        cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(sub.envMapSrvHandle)); // t2
        cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(sub.normalMapHandle)); // t3
        cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(sub.rippleTextureHandle)); // t4

        // 頂点/インデックスバッファセットして描画
        cmdList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
        cmdList->IASetIndexBuffer(&mesh.GetIndexBufferView());
        cmdList->DrawIndexedInstanced(mesh.GetIndexCount(), batch.instanceCount, 0, 0, 0);
    }
}

}