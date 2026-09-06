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
#include "Frustum.h"
#include "PIXColors.h"

namespace FE
{

void TreeRenderer::Initialize(const RenderEnvironment& env)
{
    device_ = env.device;
    auto* device = device_->GetDevice();
    auto* srvManager = env.srvManager;

    const uint32_t maxTotalInstances = kMaxInstances * kMaxPasses;
    const uint32_t maxTotalBatches = kMaxBatches * kMaxPasses;

    for (int i = 0; i < kFrameCount; ++i)
    {
        // CPUから毎フレーム全インスタンスのトランスフォームを流し込むためのUploadバッファ
        frameRes_[i].inputInstanceBuffer = BufferManager::CreateMappedBuffer<TreeInstanceData>(
            device, maxTotalInstances, &frameRes_[i].mappedInputInstanceData);

        // ComputeShaderによるカリング結果を格納するUAV
        frameRes_[i].outputInstanceBuffer = BufferManager::CreateUAVBufferResource(
            device, sizeof(TreeInstanceData) * maxTotalInstances);

        frameRes_[i].outputSrvIndex = srvManager->CreateStructuredBufferSRV(
            frameRes_[i].outputInstanceBuffer.Get(), maxTotalInstances, sizeof(TreeInstanceData));

        // GPU Driven Rendering用の間接引数バッファ
        frameRes_[i].indirectArgsBuffer = BufferManager::CreateUAVBufferResource(
            device, sizeof(AlignedDrawIndexedArguments) * maxTotalBatches);

        frameRes_[i].indirectArgsUploadBuffer = BufferManager::CreateMappedBuffer<AlignedDrawIndexedArguments>(
            device, maxTotalBatches, &frameRes_[i].mappedIndirectArgs);

        frameRes_[i].cullingDataBuffer = BufferManager::CreateMappedConstantBufferArray<TreeCullingData>(
            device, maxTotalBatches, &frameRes_[i].mappedCullingData);
    }

    // カリング用CSで消費するディスクリプタヒープを事前確保
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = maxTotalBatches * 3 * kFrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cullingHeap_));

    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc = {};
    cmdSigDesc.ByteStride = sizeof(AlignedDrawIndexedArguments);
    cmdSigDesc.NumArgumentDescs = 1;
    cmdSigDesc.pArgumentDescs = &argDesc;
    device->CreateCommandSignature(&cmdSigDesc, nullptr, IID_PPV_ARGS(&commandSignature_));
}

void TreeRenderer::BeginFrame()
{
    submissions_.clear();
    batches_.clear();
    currentInstanceLocation_ = 0;
    currentPassIndex_ = 0;

    // 非同期実行中のGPUが使用しているリソースを上書きしないようフレームをインクリメント
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

const std::vector<Mesh>& TreeRenderer::GetOrCreateBatch(const ModelData& modelData)
{
    auto it = meshCache_.find(&modelData);
    if (it != meshCache_.end()) return it->second.meshes;

    ModelBatch batch;
    batch.meshes.resize(modelData.meshes.size());

    // ジオメトリデータの初期化
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
    GetOrCreateBatch(modelData);

    // 再帰的にノード階層を走査し、ワールド行列を展開しつつリストとして抽出
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

                // メッシュIndex 0 を幹、1以降を葉
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

    // ステート切り替えコストを最小化するためのソート
    // 優先度: PSO(幹/葉) -> モデル -> メッシュ -> マテリアル
    std::sort(submissions_.begin(), submissions_.end(),
        [](const TreeSubmission& a, const TreeSubmission& b) {
            if (a.isLeaf != b.isLeaf) return a.isLeaf < b.isLeaf;
            if (a.modelData != b.modelData) return a.modelData < b.modelData;
            if (a.meshIndex != b.meshIndex) return a.meshIndex < b.meshIndex;

            if (a.treeMaterial.leafMaterialBuffer.Get() != b.treeMaterial.leafMaterialBuffer.Get())
                return a.treeMaterial.leafMaterialBuffer.Get() < b.treeMaterial.leafMaterialBuffer.Get();

            return a.treeMaterial.trunkMaterialBuffer.Get() < b.treeMaterial.trunkMaterialBuffer.Get();
        });

    uint32_t instanceCount = 0;
    currentInstanceLocation_ = 0;
    auto& curRes = frameRes_[currentFrameIndex_];

    // ソート済みの Submission を走査し、ステートが切り替わる境界でバッチを区切る（Instancingの準備）
    for (size_t i = 0; i < submissions_.size(); ++i)
    {
        const auto& sub = submissions_[i];

        if (currentInstanceLocation_ + instanceCount >= kMaxInstances) break;

        // GPUに転送するための Upload バッファへ直接書き込む
        auto& instanceGPU = curRes.mappedInputInstanceData[currentInstanceLocation_ + instanceCount];
        instanceGPU.worldMatrix = sub.worldMatrix;
        instanceGPU.colorVariation = sub.colorVariation;
        instanceGPU.lodFade = sub.lodFade;

        instanceCount++;

        bool isLast = (i == submissions_.size() - 1);
        bool shouldFlush = isLast;

        if (!isLast)
        {
            const auto& nextSub = submissions_[i + 1];
            shouldFlush = (
                sub.isLeaf != nextSub.isLeaf ||
                sub.modelData != nextSub.modelData ||
                sub.meshIndex != nextSub.meshIndex ||
                sub.treeMaterial.leafMaterialBuffer.Get() != nextSub.treeMaterial.leafMaterialBuffer.Get() ||
                sub.treeMaterial.trunkMaterialBuffer.Get() != nextSub.treeMaterial.trunkMaterialBuffer.Get()
                );
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
        }
    }
}

void TreeRenderer::Draw(const RenderEnvironment& env, ShadowMap* shadowMap, uint32_t windMapSrvIndex)
{
    if (batches_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();

    PIXScopedEvent(cmdList, FE::PIXColors::Geometry, "Tree Main Pass");

    auto& curRes = frameRes_[currentFrameIndex_];

    // 同一フレーム内でシャドウパス等とUAV/SRVが競合しないよう、パス単位でバッファ領域を分割
    uint32_t passIndex = currentPassIndex_++;
    uint32_t instanceOffset = passIndex * kMaxInstances;
    uint32_t batchOffset = passIndex * kMaxBatches;

    // GPUフラストゥムカリング用。カメラの視錐台6平面を抽出
    Frustum cameraFrustum;
    cameraFrustum.ExtractFromMatrix(viewProjectionMatrix_);

    // CBV要件である256バイトアライメントの計算
    uint32_t alignedSize = (sizeof(TreeCullingData) + 255) & ~255;
    uint8_t* ptr = reinterpret_cast<uint8_t*>(curRes.mappedCullingData) + (batchOffset * alignedSize);
    AlignedDrawIndexedArguments* mappedArgs = curRes.mappedIndirectArgs + batchOffset;

    for (size_t i = 0; i < batches_.size(); ++i)
    {
        if (i >= kMaxBatches) break;
        const auto& batch = batches_[i];
        const auto& meshes = GetOrCreateBatch(*batch.modelData);
        const Mesh* mesh = &meshes[batch.meshIndex];

        mappedArgs[i].args.IndexCountPerInstance = static_cast<uint32_t>(mesh->GetIndexCount());
        // CS内でInterlockedAddを使って可視インスタンスを積むため、描画前に必ず0クリア
        mappedArgs[i].args.InstanceCount = 0;
        mappedArgs[i].args.StartIndexLocation = 0;
        mappedArgs[i].args.BaseVertexLocation = 0;
        mappedArgs[i].args.StartInstanceLocation = batch.startInstanceLocation + instanceOffset;

        TreeCullingData cullingData = {};
        cullingData.totalInstanceCount = batch.instanceCount;
        cullingData.maxDrawDistance = this->currentMaxDrawDistance_;
        cullingData.approxTreeHeight = this->currentTreeHeight_;
        cullingData.approxTreeRadius = this->currentTreeRadius_;

        for (int p = 0; p < 6; ++p)
        {
            cullingData.frustumPlanes[p] = Vector4(
                cameraFrustum.planes[p].a, cameraFrustum.planes[p].b,
                cameraFrustum.planes[p].c, cameraFrustum.planes[p].d
            );
        }

        *reinterpret_cast<TreeCullingData*>(ptr + i * alignedSize) = cullingData;
    }

    // CPU側のUploadバッファから間接引数バッファへ初期値を転送
    D3D12_RESOURCE_BARRIER resetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        curRes.indirectArgsBuffer.Get(),
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &resetBarrier);

    cmdList->CopyBufferRegion(
        curRes.indirectArgsBuffer.Get(), batchOffset * sizeof(AlignedDrawIndexedArguments),
        curRes.indirectArgsUploadBuffer.Get(), batchOffset * sizeof(AlignedDrawIndexedArguments),
        sizeof(AlignedDrawIndexedArguments) * batches_.size());

    // カリングCSの実行に向けて、引数バッファと出力バッファをUAVステートへ遷移
    D3D12_RESOURCE_BARRIER csBarriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(curRes.indirectArgsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(curRes.outputInstanceBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    cmdList->ResourceBarrier(2, csBarriers);

    // ==========================================
    // カリングの実行
    // ==========================================
    {
        PIXScopedEvent(cmdList, FE::PIXColors::Compute, "Tree Culling CS");

        cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("TreeCullingCS"));
        cmdList->SetPipelineState(env.psoManager->GetPSO("TreeCullingCS"));

        cmdList->SetComputeRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());

        for (size_t i = 0; i < batches_.size(); ++i)
        {
            const auto& batch = batches_[i];

            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = curRes.cullingDataBuffer->GetGPUVirtualAddress() + ((batchOffset + i) * alignedSize);
            cmdList->SetComputeRootConstantBufferView(1, cbAddress);

            D3D12_GPU_VIRTUAL_ADDRESS inputSrvAddress = curRes.inputInstanceBuffer->GetGPUVirtualAddress()
                + (batch.startInstanceLocation * sizeof(TreeInstanceData));
            cmdList->SetComputeRootShaderResourceView(2, inputSrvAddress);

            D3D12_GPU_VIRTUAL_ADDRESS outputUavAddress = curRes.outputInstanceBuffer->GetGPUVirtualAddress()
                + ((batch.startInstanceLocation + instanceOffset) * sizeof(TreeInstanceData));
            cmdList->SetComputeRootUnorderedAccessView(3, outputUavAddress);

            D3D12_GPU_VIRTUAL_ADDRESS indirectUavAddress = curRes.indirectArgsBuffer->GetGPUVirtualAddress()
                + ((batchOffset + i) * sizeof(AlignedDrawIndexedArguments));
            cmdList->SetComputeRootUnorderedAccessView(4, indirectUavAddress);

            uint32_t groupX = (batch.instanceCount + 63) / 64;
            cmdList->Dispatch(groupX, 1, 1);
        }
    }

    // ==========================================
    // 描画
    // ==========================================
    // CSの書き込み完了を待ち、PSから参照可能なSRVと間接引数用ステートへバリアを張る
    D3D12_RESOURCE_BARRIER drawBarriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(curRes.outputInstanceBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(curRes.indirectArgsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
    };
    cmdList->ResourceBarrier(2, drawBarriers);

    {
        PIXScopedEvent(cmdList, FE::PIXColors::Geometry, "Tree Indirect Draw");

        ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (size_t i = 0; i < batches_.size(); ++i)
        {
            const auto& batch = batches_[i];
            const auto& meshes = GetOrCreateBatch(*batch.modelData);
            const Mesh* mesh = &meshes[batch.meshIndex];

            TreeInstanceOffset offsetData{};
            offsetData.baseInstanceIndex = batch.startInstanceLocation + instanceOffset;
            offsetData.isLeaf = batch.isLeaf ? 1u : 0u;

            if (batch.isLeaf)
            {
                cmdList->SetPipelineState(env.psoManager->GetPSO("TreeFoliage"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("TreeFoliage"));

                cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(2, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(3, batch.treeMaterial.leafMaterialBuffer->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());

                cmdList->SetGraphicsRoot32BitConstants(5, 2, &offsetData, 0);

                cmdList->SetGraphicsRootDescriptorTable(6, shadowMap->GetSRVHandle());
                cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(curRes.outputSrvIndex));
                cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));
                cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.leafTextureHandle));
                cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.leafNormalMapHandle));
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

                cmdList->SetGraphicsRoot32BitConstants(8, 2, &offsetData, 0);

                cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.trunkTextureHandle));
                cmdList->SetGraphicsRootDescriptorTable(10, shadowMap->GetSRVHandle());
                cmdList->SetGraphicsRootDescriptorTable(11, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.toonRampHandle));
                cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.trunkNormalMapHandle));
                cmdList->SetGraphicsRootDescriptorTable(13, env.srvManager->GetSRVHandleGPU(curRes.outputSrvIndex));
                cmdList->SetGraphicsRootDescriptorTable(14, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));
            }

            cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

            uint32_t argsOffset = static_cast<uint32_t>((batchOffset + i) * sizeof(AlignedDrawIndexedArguments));
            cmdList->ExecuteIndirect(commandSignature_.Get(), 1, curRes.indirectArgsBuffer.Get(), argsOffset, nullptr, 0);
        }
    }
}


void TreeRenderer::DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex, uint32_t windMapSrvIndex)
{
    if (batches_.empty() || currentPassIndex_ >= kMaxPasses) return;

    auto* cmdList = env.commandManager->GetCommandList();

    PIXScopedEvent(cmdList, FE::PIXColors::Shadow, "Tree Shadow Pass (Cascade %u)", cascadeIndex);

    auto& curRes = frameRes_[currentFrameIndex_];

    uint32_t passIndex = currentPassIndex_++;
    uint32_t instanceOffset = passIndex * kMaxInstances;
    uint32_t batchOffset = passIndex * kMaxBatches;

    // カスケードレベルに応じたライト視錐台を抽出し、シャドウ領域外の木を間引く
    const ShadowData* shadowData = env.lightManager->GetShadowData();
    Frustum lightFrustum;
    lightFrustum.ExtractFromMatrix(shadowData->cascadeLightViewProj[cascadeIndex]);

    uint32_t alignedSize = (sizeof(TreeCullingData) + 255) & ~255;
    uint8_t* cullingPtr = reinterpret_cast<uint8_t*>(curRes.mappedCullingData) + (batchOffset * alignedSize);
    AlignedDrawIndexedArguments* mappedArgs = curRes.mappedIndirectArgs + batchOffset;

    for (size_t i = 0; i < batches_.size(); ++i)
    {
        if (i >= kMaxBatches) break;
        const auto& batch = batches_[i];
        const auto& meshes = GetOrCreateBatch(*batch.modelData);
        const Mesh* mesh = &meshes[batch.meshIndex];

        mappedArgs[i].args.IndexCountPerInstance = static_cast<uint32_t>(mesh->GetIndexCount());
        mappedArgs[i].args.InstanceCount = 0;
        mappedArgs[i].args.StartIndexLocation = 0;
        mappedArgs[i].args.BaseVertexLocation = 0;
        mappedArgs[i].args.StartInstanceLocation = batch.startInstanceLocation + instanceOffset;

        TreeCullingData cullingData = {};
        cullingData.totalInstanceCount = batch.instanceCount;
        cullingData.maxDrawDistance = this->currentMaxDrawDistance_;
        cullingData.approxTreeHeight = this->currentTreeHeight_;
        cullingData.approxTreeRadius = this->currentTreeRadius_;

        for (int p = 0; p < 6; ++p) {
            cullingData.frustumPlanes[p] = Vector4(
                lightFrustum.planes[p].a, lightFrustum.planes[p].b,
                lightFrustum.planes[p].c, lightFrustum.planes[p].d
            );
        }
        *reinterpret_cast<TreeCullingData*>(cullingPtr + i * alignedSize) = cullingData;
    }

    D3D12_RESOURCE_BARRIER resetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        curRes.indirectArgsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &resetBarrier);

    cmdList->CopyBufferRegion(
        curRes.indirectArgsBuffer.Get(), batchOffset * sizeof(AlignedDrawIndexedArguments),
        curRes.indirectArgsUploadBuffer.Get(), batchOffset * sizeof(AlignedDrawIndexedArguments),
        sizeof(AlignedDrawIndexedArguments) * batches_.size());

    D3D12_RESOURCE_BARRIER csBarriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(curRes.indirectArgsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(curRes.outputInstanceBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    cmdList->ResourceBarrier(2, csBarriers);

    // ==========================================
    // カリング CS 実行
    // ==========================================
    {
        PIXScopedEvent(cmdList, FE::PIXColors::Compute, "Tree Shadow Culling CS");

        cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("TreeCullingCS"));
        cmdList->SetPipelineState(env.psoManager->GetPSO("TreeCullingCS"));

        cmdList->SetComputeRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());

        for (size_t i = 0; i < batches_.size(); ++i)
        {
            const auto& batch = batches_[i];

            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = curRes.cullingDataBuffer->GetGPUVirtualAddress() + ((batchOffset + i) * alignedSize);
            cmdList->SetComputeRootConstantBufferView(1, cbAddress);

            D3D12_GPU_VIRTUAL_ADDRESS inputSrvAddress = curRes.inputInstanceBuffer->GetGPUVirtualAddress()
                + (batch.startInstanceLocation * sizeof(TreeInstanceData));
            cmdList->SetComputeRootShaderResourceView(2, inputSrvAddress);

            D3D12_GPU_VIRTUAL_ADDRESS outputUavAddress = curRes.outputInstanceBuffer->GetGPUVirtualAddress()
                + ((batch.startInstanceLocation + instanceOffset) * sizeof(TreeInstanceData));
            cmdList->SetComputeRootUnorderedAccessView(3, outputUavAddress);

            D3D12_GPU_VIRTUAL_ADDRESS indirectUavAddress = curRes.indirectArgsBuffer->GetGPUVirtualAddress()
                + ((batchOffset + i) * sizeof(AlignedDrawIndexedArguments));
            cmdList->SetComputeRootUnorderedAccessView(4, indirectUavAddress);

            uint32_t groupX = (batch.instanceCount + 63) / 64;
            cmdList->Dispatch(groupX, 1, 1);
        }
    }

    // ==========================================
    // 描画フェーズへのバリア
    // ==========================================
    D3D12_RESOURCE_BARRIER drawBarriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(curRes.outputInstanceBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(curRes.indirectArgsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
    };
    cmdList->ResourceBarrier(2, drawBarriers);

    // ==========================================
    // シャドウマップ描画
    // ==========================================
    {
        PIXScopedEvent(cmdList, FE::PIXColors::Shadow, "Tree Shadow Indirect Draw");

        ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (size_t i = 0; i < batches_.size(); ++i)
        {
            const auto& batch = batches_[i];
            const auto& meshes = GetOrCreateBatch(*batch.modelData);
            const Mesh* mesh = &meshes[batch.meshIndex];

            TreeInstanceOffset offsetData{};
            offsetData.baseInstanceIndex = batch.startInstanceLocation + instanceOffset;
            offsetData.isLeaf = batch.isLeaf ? 1u : 0u;

            if (batch.isLeaf)
            {
                cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapTreeFoliage"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapTreeFoliage"));

                cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(1, batch.treeMaterial.leafMaterialBuffer->GetGPUVirtualAddress());
                cmdList->SetGraphicsRoot32BitConstants(2, 2, &offsetData, 0);
                cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRoot32BitConstant(4, cascadeIndex, 0);
                cmdList->SetGraphicsRootConstantBufferView(5, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());

                cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(curRes.outputSrvIndex));
                cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));
                cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.leafTextureHandle));
            }
            else
            {
                cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapTreeTrunk"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapTreeTrunk"));

                cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(1, batch.treeMaterial.trunkMaterialBuffer->GetGPUVirtualAddress());
                cmdList->SetGraphicsRoot32BitConstants(2, 2, &offsetData, 0);
                cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRoot32BitConstant(4, cascadeIndex, 0);
                cmdList->SetGraphicsRootConstantBufferView(5, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());

                cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(curRes.outputSrvIndex));
                cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));
            }

            cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

            uint32_t argsOffset = static_cast<uint32_t>((batchOffset + i) * sizeof(AlignedDrawIndexedArguments));
            cmdList->ExecuteIndirect(commandSignature_.Get(), 1, curRes.indirectArgsBuffer.Get(), argsOffset, nullptr, 0);
        }
    }
}

void TreeRenderer::SetCullingParameters(float maxDrawDistance, float treeHeight, float treeRadius)
{
    currentMaxDrawDistance_ = maxDrawDistance;
    currentTreeHeight_ = treeHeight;
    currentTreeRadius_ = treeRadius;
}

}