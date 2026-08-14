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

    // 1. 生成用全域バッファの作成
    generatedPebbleBuffer_ = BufferManager::CreateUAVBufferResource(
        device, sizeof(PebbleInstanceData) * kMaxInstances);

    generatedSrvIndex_ = srvManager->CreateStructuredBufferSRV(
        generatedPebbleBuffer_.Get(), kMaxInstances, sizeof(PebbleInstanceData));
    generatedUavIndex_ = srvManager->CreateStructuredBufferUAV(
        generatedPebbleBuffer_.Get(), kMaxInstances, sizeof(PebbleInstanceData));

    // 2. カリング用ディスクリプタヒープの作成
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 3 * kFrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cullingHeap_));

    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 3. フレームごとのバッファ生成
    for (int i = 0; i < kFrameCount; ++i)
    {
        // ★ 定数バッファのマルチバッファ化
        generationDataResource_[i] = BufferManager::CreateMappedConstantBuffer<PebbleGenerationData>(device, &mappedGenData_[i]);
        materialResource_[i] = BufferManager::CreateMappedConstantBuffer<PebbleMaterialData>(device, &mappedMaterial_[i]);
        cullingDataResource_[i] = BufferManager::CreateMappedConstantBuffer<PebbleCullingData>(device, &mappedCullingData_[i]);

        outputInstanceBuffer_[i] = BufferManager::CreateUAVBufferResource(device, sizeof(PebbleInstanceData) * kMaxInstances);
        indirectArgsBuffer_[i] = BufferManager::CreateUAVBufferResource(device, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

        // ★ アップロードバッファもフレーム分作成し、マップを保持
        indirectArgsUploadBuffer_[i] = BufferManager::CreateMappedBuffer<D3D12_DRAW_INDEXED_ARGUMENTS>(device, 1, &mappedArgs_[i]);
        *mappedArgs_[i] = {}; // ゼロクリア

        outputUavIndex_[i] = srvManager->CreateStructuredBufferUAV(outputInstanceBuffer_[i].Get(), kMaxInstances, sizeof(PebbleInstanceData));
        outputSrvIndex_[i] = srvManager->CreateStructuredBufferSRV(outputInstanceBuffer_[i].Get(), kMaxInstances, sizeof(PebbleInstanceData));
        indirectUavIndex_[i] = srvManager->CreateRawBufferUAV(indirectArgsBuffer_[i].Get(), sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

        // ★ [最適化] Initializeの時点でCullingHeapへディスクリプタをコピーしておく
        UINT slotOffset = i * 3;
        D3D12_CPU_DESCRIPTOR_HANDLE destCPU = cullingHeap_->GetCPUDescriptorHandleForHeapStart();
        destCPU.ptr += slotOffset * handleSize;

        device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(generatedSrvIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        destCPU.ptr += handleSize;
        device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(outputUavIndex_[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        destCPU.ptr += handleSize;
        device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(indirectUavIndex_[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    // 4. Command Signature の構築 (変更なし)
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

    // ★ フレームの定数バッファへコピー
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

    // ★ 現在のフレームのGPUアドレスをバインド
    cmdList->SetComputeRootConstantBufferView(0, generationDataResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(1, terrainSettingsAddress);
    cmdList->SetComputeRootDescriptorTable(2, env.srvManager->GetSRVHandleGPU(heightMapSrvHandle));
    cmdList->SetComputeRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(densityMapSrvHandle));
    cmdList->SetComputeRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(generatedUavIndex_));

    // ★ 1D Dispatchに変更
    UINT totalThreads = genData.maxInstancesPerChunk;
    UINT maxGroupsX = 1024; // 1024グループ * 64スレッド = 65536スレッド
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
    const Mesh& pebbleMesh, // ★ 単一メッシュ
    const PebbleMaterialData& materialData,
    const PebbleCullingData& cullingData)
{
    if (totalGeneratedCount_ == 0) return;

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12Device* device = env.device->GetDevice();

    memcpy(mappedMaterial_[currentFrameIndex_], &materialData, sizeof(PebbleMaterialData));

    // ==========================================
    // パス 1: GPU カリング (Compute Shader)
    // ==========================================
    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("PebbleCullingCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("PebbleCullingCS"));

    ID3D12DescriptorHeap* cullingHeaps[] = { cullingHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, cullingHeaps);
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 1. 引数のセットアップ (Map/Unmapは不要になり、フレーム固有バッファに直書き)
    D3D12_DRAW_INDEXED_ARGUMENTS drawArgs = {};
    drawArgs.IndexCountPerInstance = pebbleMesh.GetIndexCount();
    drawArgs.InstanceCount = 0; // ★ CSで加算されるので必ず 0 初期化
    drawArgs.StartIndexLocation = 0;
    drawArgs.BaseVertexLocation = 0;
    drawArgs.StartInstanceLocation = 0;

    *mappedArgs_[currentFrameIndex_] = drawArgs; // アップロードバッファへ反映

    D3D12_RESOURCE_BARRIER resetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        indirectArgsBuffer_[currentFrameIndex_].Get(),
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &resetBarrier);

    // フレーム固有のアップロードバッファから VRAM へコピー
    cmdList->CopyBufferRegion(
        indirectArgsBuffer_[currentFrameIndex_].Get(), 0,
        indirectArgsUploadBuffer_[currentFrameIndex_].Get(), 0,
        sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

    // 2. バリア遷移
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

    // 3. CullingData の更新
    PebbleCullingData actualCullingData = cullingData;
    actualCullingData.totalInstanceCount = totalGeneratedCount_;
    memcpy(mappedCullingData_[currentFrameIndex_], &actualCullingData, sizeof(PebbleCullingData));

    // 4. ディスクリプタのバインド (★ Initialize でコピー済みなので Set だけ)
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

    // 5. カリング Dispatch (★ 1D化)
    UINT totalThreads = totalGeneratedCount_;
    UINT maxGroupsX = 1024;
    UINT dispatchX = std::min((totalThreads + 63) / 64, maxGroupsX);
    UINT dispatchY = (totalThreads + 65535) / 65536;
    cmdList->Dispatch(dispatchX, dispatchY, 1);

    // ==========================================
    // パス 2: 間接描画 (ExecuteIndirect)
    // ==========================================
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

    // 定数バッファ・テクスチャのバインド
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

    cmdList->SetGraphicsRootShaderResourceView(6, outputInstanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress());

    cmdList->ExecuteIndirect(
        commandSignature_.Get(), 1,
        indirectArgsBuffer_[currentFrameIndex_].Get(), 0, nullptr, 0);
}

}