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
    auto* device = device_->GetDevice();
    auto* srvManager = env.srvManager;

    // 1. CPU側の入力バッファ (全インスタンス配置用)
    instanceBuffer_.resource = BufferManager::CreateMappedBuffer(
        device, kMaxInstances, &instanceBuffer_.mapped);

    // 2. ExecuteIndirect用引数のUploadバッファ (バッチ数分確保)
    indirectArgsUploadBuffer_ = BufferManager::CreateMappedBuffer<AlignedDrawIndexedArguments>(
        device, kMaxBatches, &mappedIndirectArgs_);

    // 3. フレームリソースの確保
    for (int i = 0; i < kFrameCount; ++i)
    {
        // カリング後の可視インスタンスを書き込むUAVバッファ
        frameRes_[i].outputInstanceBuffer = BufferManager::CreateUAVBufferResource(
            device, sizeof(TreeInstanceData) * kMaxInstances);

        // 全体のSRV (VS描画用: ExecuteIndirectのStartInstanceLocationにより先頭から読まれる)
        frameRes_[i].outputSrvIndex = srvManager->CreateStructuredBufferSRV(
            frameRes_[i].outputInstanceBuffer.Get(), kMaxInstances, sizeof(TreeInstanceData));

        // 間接描画引数バッファ (GPUが直接書き込む)
        frameRes_[i].indirectArgsBuffer = BufferManager::CreateUAVBufferResource(
            device, sizeof(AlignedDrawIndexedArguments) * kMaxBatches);

        // カリング用定数バッファ (バッチごとにカリングデータが異なる場合を想定しバッチ数分確保)
        frameRes_[i].cullingDataBuffer = BufferManager::CreateMappedConstantBufferArrayPacked<TreeCullingData>(
            device, kMaxBatches, &frameRes_[i].mappedCullingData);
    }

    // 4. Culling CS用ローカルヒープ (1フレームあたり バッチ数 × 3(SRV1+UAV2) の余裕を持たせる)
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = kMaxBatches * 3 * kFrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cullingHeap_));

    // 5. コマンドシグネチャの作成 (DrawIndexed用)
    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED; // 木はメッシュなのでIndexed

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
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
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
    ID3D12Device* device = device_->GetDevice();
    auto& curRes = frameRes_[currentFrameIndex_];

    // ==========================================
    // 1. カリングフェーズの準備
    // ==========================================

    // CPU側でバッチごとの間接引数(初期値)とカリング定数を設定
    for (size_t i = 0; i < batches_.size(); ++i)
    {
        if (i >= kMaxBatches) break;

        const auto& batch = batches_[i];
        const auto& meshes = GetOrCreateBatch(*batch.modelData);
        const Mesh* mesh = &meshes[batch.meshIndex];

        // 描画引数の初期値設定 (InstanceCountは0にしてGPUでアトミック加算させる)
        mappedIndirectArgs_[i].args.IndexCountPerInstance = static_cast<UINT>(mesh->GetIndexCount());
        mappedIndirectArgs_[i].args.InstanceCount = 0;
        mappedIndirectArgs_[i].args.StartIndexLocation = 0;
        mappedIndirectArgs_[i].args.BaseVertexLocation = 0;
        mappedIndirectArgs_[i].args.StartInstanceLocation = batch.startInstanceLocation; // ★VSで正確なインデックスから読ませるため重要

        // Culling CS用のデータ構築
        TreeCullingData cullingData = {};
        cullingData.totalInstanceCount = batch.instanceCount;
        cullingData.maxDrawDistance = this->currentMaxDrawDistance_;
        cullingData.approxTreeHeight = this->currentTreeHeight_;
        cullingData.approxTreeRadius = this->currentTreeRadius_;

        UINT alignedSize = (sizeof(TreeCullingData) + 255) & ~255;
        uint8_t* ptr = reinterpret_cast<uint8_t*>(curRes.mappedCullingData);
        *reinterpret_cast<TreeCullingData*>(ptr + i * alignedSize) = cullingData;
    }

    // 間接引数バッファへ初期値をコピーするためのバリア
    D3D12_RESOURCE_BARRIER resetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        curRes.indirectArgsBuffer.Get(),
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &resetBarrier);

    cmdList->CopyBufferRegion(
        curRes.indirectArgsBuffer.Get(), 0,
        indirectArgsUploadBuffer_.Get(), 0,
        sizeof(AlignedDrawIndexedArguments) * batches_.size());

    // バッファをUAVステートへ遷移
    D3D12_RESOURCE_BARRIER csBarriers[2] = {};
    csBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        curRes.indirectArgsBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    csBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
        curRes.outputInstanceBuffer.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(2, csBarriers);

    // ==========================================
    // 2. カリングの実行 (Compute Shader)
    // ==========================================
    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("TreeCullingCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("TreeCullingCS"));

    ID3D12DescriptorHeap* cullingHeaps[] = { cullingHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, cullingHeaps);

    UINT handleIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE destCPU = cullingHeap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE destGPU = cullingHeap_->GetGPUDescriptorHandleForHeapStart();

    // フレームごとにディスクリプタの書き込み領域をずらす
    UINT frameOffset = kMaxBatches * 3 * currentFrameIndex_;
    destCPU.ptr += frameOffset * handleIncSize;
    destGPU.ptr += frameOffset * handleIncSize;

    cmdList->SetComputeRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());

    for (size_t i = 0; i < batches_.size(); ++i)
    {
        const auto& batch = batches_[i];

        // 定数バッファのバインド
        UINT alignedSize = (sizeof(TreeCullingData) + 255) & ~255;
        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = curRes.cullingDataBuffer->GetGPUVirtualAddress() + i * alignedSize;
        cmdList->SetComputeRootConstantBufferView(1, cbAddress);

        // ★ バッチごとに FirstElement をずらした View を動的に作成する ★
        // これにより、HLSL側は一切コードを変更せずに 0 から処理を行える

        // SRV0: 入力バッファ
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = batch.startInstanceLocation;
        srvDesc.Buffer.NumElements = batch.instanceCount;
        srvDesc.Buffer.StructureByteStride = sizeof(TreeInstanceData);
        device->CreateShaderResourceView(instanceBuffer_.resource.Get(), &srvDesc, destCPU);
        destCPU.ptr += handleIncSize;

        // UAV0: 出力バッファ
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc0 = {};
        uavDesc0.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc0.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc0.Buffer.FirstElement = batch.startInstanceLocation;
        uavDesc0.Buffer.NumElements = batch.instanceCount;
        uavDesc0.Buffer.StructureByteStride = sizeof(TreeInstanceData);
        device->CreateUnorderedAccessView(curRes.outputInstanceBuffer.Get(), nullptr, &uavDesc0, destCPU);
        destCPU.ptr += handleIncSize;

        // UAV1: 引数バッファ
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc1 = {};
        uavDesc1.Format = DXGI_FORMAT_R32_TYPELESS;
        uavDesc1.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        // ByteAddressBuffer は 4バイト単位で FirstElement を指定する
        uavDesc1.Buffer.FirstElement = static_cast<UINT>(i * sizeof(AlignedDrawIndexedArguments) / 4);
        uavDesc1.Buffer.NumElements = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS) / 4;
        uavDesc1.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        device->CreateUnorderedAccessView(curRes.indirectArgsBuffer.Get(), nullptr, &uavDesc1, destCPU);
        destCPU.ptr += handleIncSize;

        // Tableのバインド (RootSignatureの設定に合わせて適宜インデックス調整)
        // 仮に [2] が SRV, [3] が Output UAV, [4] が Args UAV とする
        cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 0, handleIncSize));
        cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 1, handleIncSize));
        cmdList->SetComputeRootDescriptorTable(4, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 2, handleIncSize));
        destGPU.ptr += handleIncSize * 3;

        UINT groupX = (batch.instanceCount + 63) / 64;
        cmdList->Dispatch(groupX, 1, 1);
    }

    // ==========================================
    // 3. 描画フェーズ (ExecuteIndirect)
    // ==========================================

    // カリング終了後、SRVとIndirectArgumentへ遷移
    D3D12_RESOURCE_BARRIER drawBarriers[2] = {};
    drawBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        curRes.outputInstanceBuffer.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    drawBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
        curRes.indirectArgsBuffer.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    cmdList->ResourceBarrier(2, drawBarriers);

    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (size_t i = 0; i < batches_.size(); ++i)
    {
        const auto& batch = batches_[i];
        const auto& meshes = GetOrCreateBatch(*batch.modelData);
        const Mesh* mesh = &meshes[batch.meshIndex];

        TreeInstanceOffset offsetData{};
        offsetData.baseInstanceIndex = batch.startInstanceLocation;
        offsetData.isLeaf = batch.isLeaf ? 1u : 0u;

        // --- 幹と葉のPSOおよびリソースバインド (既存のコードとほぼ同一) ---
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

            cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.envMapHandle));
            cmdList->SetGraphicsRootDescriptorTable(7, shadowMap->GetSRVHandle());
            cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(curRes.outputSrvIndex));
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

            cmdList->SetGraphicsRoot32BitConstants(8, 2, &offsetData, 0);

            cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.trunkTextureHandle));
            cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.envMapHandle));
            cmdList->SetGraphicsRootDescriptorTable(11, shadowMap->GetSRVHandle());
            cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.toonRampHandle));
            cmdList->SetGraphicsRootDescriptorTable(13, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.trunkNormalMapHandle));
            cmdList->SetGraphicsRootDescriptorTable(14, env.srvManager->GetSRVHandleGPU(curRes.outputSrvIndex));
            cmdList->SetGraphicsRootDescriptorTable(15, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));

        }

        cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

        // ★ DrawIndexedInstanced の代わりに ExecuteIndirect で描画
        UINT argsOffset = static_cast<UINT>(i * sizeof(AlignedDrawIndexedArguments));

        cmdList->ExecuteIndirect(
            commandSignature_.Get(),
            1,
            curRes.indirectArgsBuffer.Get(),
            argsOffset,
            nullptr,
            0);
    }
}
//
//void TreeRenderer::Draw(const RenderEnvironment& env, ShadowMap* shadowMap, uint32_t windMapSrvIndex)
//{
//    if (batches_.empty()) return;
//
//    auto* cmdList = env.commandManager->GetCommandList();
//    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
//    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
//    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//
//    for (const auto& batch : batches_)
//    {
//        const auto& meshes = GetOrCreateBatch(*batch.modelData);
//        const Mesh* mesh = &meshes[batch.meshIndex];
//        uint32_t indexCount = static_cast<uint32_t>(mesh->GetIndexCount());
//
//        // ★ 1. HLSLへ送るルート定数(Push Constants)データを作成
//        TreeInstanceOffset offsetData{};
//        offsetData.baseInstanceIndex = batch.startInstanceLocation;
//        offsetData.isLeaf = batch.isLeaf ? 1u : 0u;
//
//        if (batch.isLeaf)
//        {
//            cmdList->SetPipelineState(env.psoManager->GetPSO("TreeFoliage"));
//            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("TreeFoliage"));
//
//            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
//            cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
//            cmdList->SetGraphicsRootConstantBufferView(2, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());
//            cmdList->SetGraphicsRootConstantBufferView(3, batch.treeMaterial.leafMaterialBuffer->GetGPUVirtualAddress());
//            cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
//
//            cmdList->SetGraphicsRoot32BitConstants(5, 2, &offsetData, 0);
//
//            cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.envMapHandle));
//            cmdList->SetGraphicsRootDescriptorTable(7, shadowMap->GetSRVHandle());
//            cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));
//            cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));
//            // ★葉のテクスチャ
//            cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.leafTextureHandle));
//            cmdList->SetGraphicsRootDescriptorTable(11, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.leafNormalMapHandle));
//            cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.leafMetallicRoughnessHandle));
//        }
//        else
//        {
//            cmdList->SetPipelineState(env.psoManager->GetPSO("TreeTrunk"));
//            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("TreeTrunk"));
//
//            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
//            cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
//            cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetPointLightResource()->GetGPUVirtualAddress());
//            cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetSpotLightResource()->GetGPUVirtualAddress());
//            cmdList->SetGraphicsRootConstantBufferView(4, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());
//
//            cmdList->SetGraphicsRootConstantBufferView(5, batch.treeMaterial.leafMaterialBuffer->GetGPUVirtualAddress());
//            cmdList->SetGraphicsRootConstantBufferView(6, batch.treeMaterial.trunkMaterialBuffer->GetGPUVirtualAddress());
//            cmdList->SetGraphicsRootConstantBufferView(7, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
//
//            cmdList->SetGraphicsRoot32BitConstants(8, 2, &offsetData, 0);
//
//            cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.trunkTextureHandle));
//            cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.envMapHandle));
//            cmdList->SetGraphicsRootDescriptorTable(11, shadowMap->GetSRVHandle());
//            cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.toonRampHandle));
//            cmdList->SetGraphicsRootDescriptorTable(13, env.srvManager->GetSRVHandleGPU(batch.treeMaterial.trunkNormalMapHandle));
//            cmdList->SetGraphicsRootDescriptorTable(14, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));
//            cmdList->SetGraphicsRootDescriptorTable(15, env.srvManager->GetSRVHandleGPU(windMapSrvIndex));
//        }
//
//        cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
//        cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
//        cmdList->DrawIndexedInstanced(indexCount, batch.instanceCount, 0, 0, 0);
//    }
//}

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

        // ★ 1. HLSLへ送るルート定数データを作成
        TreeInstanceOffset offsetData{};
        offsetData.baseInstanceIndex = batch.startInstanceLocation;
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
            cmdList->SetGraphicsRoot32BitConstants(2, 2, &offsetData, 0);
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

void TreeRenderer::SetCullingParameters(float maxDrawDistance, float treeHeight, float treeRadius)
{
    currentMaxDrawDistance_ = maxDrawDistance;
    currentTreeHeight_ = treeHeight;
    currentTreeRadius_ = treeRadius;
}

}