#include "pch.h"
#include "PebbleRenderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"

namespace FE
{

void PebbleRenderer::Initialize(const RenderEnvironment& env)
{
    ID3D12Device* device = env.device->GetDevice();
    auto* srvManager = env.srvManager;

    generatedPebbleBuffer_ = BufferManager::CreateUAVBufferResource(
        device, sizeof(PebbleInstanceData) * kMaxInstances);

    generatedSrvIndex_ = srvManager->CreateStructuredBufferSRV(
        generatedPebbleBuffer_.Get(), kMaxInstances, sizeof(PebbleInstanceData));
    generatedUavIndex_ = srvManager->CreateStructuredBufferUAV(
        generatedPebbleBuffer_.Get(), kMaxInstances, sizeof(PebbleInstanceData));

    // カリング用の専用ヒープを確保
    // 専用のシェーダービジブルヒープを作成し、ポインタを切り替えるだけで済ませる
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 3 * kFrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cullingHeap_));

    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (int i = 0; i < kFrameCount; ++i)
    {
        generationDataResource_[i] = BufferManager::CreateMappedConstantBuffer<PebbleGenerationData>(device, &mappedGenData_[i]);
        materialResource_[i] = BufferManager::CreateMappedConstantBuffer<PebbleMaterialData>(device, &mappedMaterial_[i]);
        cullingDataResource_[i] = BufferManager::CreateMappedConstantBuffer<PebbleCullingData>(device, &mappedCullingData_[i]);

        outputInstanceBuffer_[i] = BufferManager::CreateUAVBufferResource(device, sizeof(PebbleInstanceData) * kMaxInstances);
        indirectArgsBuffer_[i] = BufferManager::CreateUAVBufferResource(device, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

        indirectArgsUploadBuffer_[i] = BufferManager::CreateMappedBuffer<D3D12_DRAW_INDEXED_ARGUMENTS>(device, 1, &mappedArgs_[i]);
        *mappedArgs_[i] = {};

        outputUavIndex_[i] = srvManager->CreateStructuredBufferUAV(outputInstanceBuffer_[i].Get(), kMaxInstances, sizeof(PebbleInstanceData));
        outputSrvIndex_[i] = srvManager->CreateStructuredBufferSRV(outputInstanceBuffer_[i].Get(), kMaxInstances, sizeof(PebbleInstanceData));
        indirectUavIndex_[i] = srvManager->CreateRawBufferUAV(indirectArgsBuffer_[i].Get(), sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

        // ドローコール時の CPU オーバーヘッドをゼロにするため、
        // 初期化フェーズで予め全フレーム分のディスクリプタをヒープに焼いておく
        UINT slotOffset = i * 3;
        D3D12_CPU_DESCRIPTOR_HANDLE destCPU = cullingHeap_->GetCPUDescriptorHandleForHeapStart();
        destCPU.ptr += slotOffset * handleSize;

        device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(generatedSrvIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        destCPU.ptr += handleSize;
        device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(outputUavIndex_[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        destCPU.ptr += handleSize;
        device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(indirectUavIndex_[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
    D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc = {};
    cmdSigDesc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
    cmdSigDesc.NumArgumentDescs = 1;
    cmdSigDesc.pArgumentDescs = &argDesc;
    device->CreateCommandSignature(&cmdSigDesc, nullptr, IID_PPV_ARGS(&commandSignature_));
}

void PebbleRenderer::BeginFrame()
{
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void PebbleRenderer::GeneratePebbles(
    const RenderEnvironment& env,
    const PebbleGenerationData& genData,
    uint32_t heightMapSrvHandle,
    uint32_t densityMapSrvHandle,
    D3D12_GPU_VIRTUAL_ADDRESS terrainSettingsAddress)
{
    auto* cmdList = env.commandManager->GetCommandList();

    // 初期化時にMap済みのポインタへのmemcpyだけで済むよう設計
    memcpy(mappedGenData_[currentFrameIndex_], &genData, sizeof(PebbleGenerationData));

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        generatedPebbleBuffer_.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("PebbleGenerationCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("PebbleGenerationCS"));

    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetComputeRootConstantBufferView(0, generationDataResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(1, terrainSettingsAddress);
    cmdList->SetComputeRootDescriptorTable(2, env.srvManager->GetSRVHandleGPU(heightMapSrvHandle));
    cmdList->SetComputeRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(densityMapSrvHandle));
    cmdList->SetComputeRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(generatedUavIndex_));

    // D3D12のDispatchのX最大値を超える大量のインスタンス生成に対応
    UINT totalThreads = genData.maxInstancesPerChunk;
    UINT maxGroupsX = 1024; 
    UINT dispatchX = std::min((totalThreads + 63) / 64, maxGroupsX);
    UINT dispatchY = (totalThreads + 65535) / 65536;

    cmdList->Dispatch(dispatchX, dispatchY, 1);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        generatedPebbleBuffer_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);

    totalGeneratedCount_ = genData.maxInstancesPerChunk;
}

void PebbleRenderer::Draw(
    const RenderEnvironment& env,
    ShadowMap* shadowMap,
    uint32_t skyboxSrvHandle,
    uint32_t albedoSrvHandle,
    uint32_t normalSrvHandle,
    const Mesh& pebbleMesh,
    const PebbleMaterialData& materialData,
    const PebbleCullingData& cullingData)
{
    if (totalGeneratedCount_ == 0) return;

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12Device* device = env.device->GetDevice();

    memcpy(mappedMaterial_[currentFrameIndex_], &materialData, sizeof(PebbleMaterialData));

    // ==========================================
    // パス 1: GPU カリング 
    // ==========================================
    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("PebbleCullingCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("PebbleCullingCS"));

    ID3D12DescriptorHeap* cullingHeaps[] = { cullingHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, cullingHeaps);
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // IndirectDrawのインスタンス数はCS側で加算するため、
    // 毎フレーム描画前に必ず 0 に初期化しておく
    D3D12_DRAW_INDEXED_ARGUMENTS drawArgs = {};
    drawArgs.IndexCountPerInstance = static_cast<UINT>(pebbleMesh.GetIndexCount());
    drawArgs.InstanceCount = 0;
    drawArgs.StartIndexLocation = 0;
    drawArgs.BaseVertexLocation = 0;
    drawArgs.StartInstanceLocation = 0;

    *mappedArgs_[currentFrameIndex_] = drawArgs;

    D3D12_RESOURCE_BARRIER resetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        indirectArgsBuffer_[currentFrameIndex_].Get(),
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &resetBarrier);

    // CPU側のUploadバッファからGPU側バッファへ描画引数を高速転送
    cmdList->CopyBufferRegion(
        indirectArgsBuffer_[currentFrameIndex_].Get(), 0,
        indirectArgsUploadBuffer_[currentFrameIndex_].Get(), 0,
        sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

    // カリングCSの実行に備え、UAVおよび間接引数バッファの状態を遷移
    D3D12_RESOURCE_BARRIER csBarriers[2] = {};
    csBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        outputInstanceBuffer_[currentFrameIndex_].Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    csBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
        indirectArgsBuffer_[currentFrameIndex_].Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(2, csBarriers);

    PebbleCullingData actualCullingData = cullingData;
    actualCullingData.totalInstanceCount = totalGeneratedCount_;
    memcpy(mappedCullingData_[currentFrameIndex_], &actualCullingData, sizeof(PebbleCullingData));

    // 毎フレームの Descriptor Table の書き込みを排除し、
    // 初期化時にベイク済みの専用ディスクリプタヒープ領域のGPUハンドルを直接指定
    UINT slotOffset = currentFrameIndex_ * 3;
    D3D12_GPU_DESCRIPTOR_HANDLE destGPU = cullingHeap_->GetGPUDescriptorHandleForHeapStart();
    destGPU.ptr += slotOffset * handleSize;

    cmdList->SetComputeRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(1, cullingDataResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(2, destGPU);
    destGPU.ptr += handleSize;
    cmdList->SetComputeRootDescriptorTable(3, destGPU);
    destGPU.ptr += handleSize;
    cmdList->SetComputeRootDescriptorTable(4, destGPU);

    UINT totalThreads = totalGeneratedCount_;
    UINT maxGroupsX = 1024;
    UINT dispatchX = std::min((totalThreads + 63) / 64, maxGroupsX);
    UINT dispatchY = (totalThreads + 65535) / 65536;
    cmdList->Dispatch(dispatchX, dispatchY, 1);

    // ==========================================
    // パス 2: 間接描画
    // ==========================================
    // CSの書き込み完了を待ち、PSから参照可能なSRV状態および間接描画引数状態へバリアを張る
    D3D12_RESOURCE_BARRIER drawBarriers[2] = {};
    drawBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        outputInstanceBuffer_[currentFrameIndex_].Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    drawBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
        indirectArgsBuffer_[currentFrameIndex_].Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    cmdList->ResourceBarrier(2, drawBarriers);

    cmdList->SetPipelineState(env.psoManager->GetPSO("Pebble"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Pebble"));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D12DescriptorHeap* mainHeaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, mainHeaps);

    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetPointLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetSpotLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(4, materialResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(5, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(skyboxSrvHandle));
    cmdList->SetGraphicsRootDescriptorTable(8, shadowMap->GetSRVHandle());
    cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(albedoSrvHandle));
    cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(normalSrvHandle));

    cmdList->IASetVertexBuffers(0, 1, &pebbleMesh.GetVertexBufferView());
    cmdList->IASetIndexBuffer(&pebbleMesh.GetIndexBufferView());

    // カリング済みの可視インスタンス配列を構造化バッファとして頂点シェーダーに供給
    cmdList->SetGraphicsRootShaderResourceView(6, outputInstanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress());

    // GPU側で計算された描画数をもとに、CPUを介さず直接ドローを発行
    cmdList->ExecuteIndirect(
        commandSignature_.Get(), 1,
        indirectArgsBuffer_[currentFrameIndex_].Get(), 0, nullptr, 0);
}

}