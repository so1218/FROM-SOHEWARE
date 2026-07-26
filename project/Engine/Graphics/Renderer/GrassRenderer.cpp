#include "pch.h"
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

namespace FE
{

void GrassRenderer::Initialize(const RenderEnvironment& env)
{
    ID3D12Device* device = env.device->GetDevice();
    auto* srvManager = env.srvManager;

    // 1. 間接描画引数の初期化バッファ作成 (VertexCount=8, InstanceCount=0, StartVertex=0, StartInstance=0)
    D3D12_DRAW_ARGUMENTS drawArgs = { 8, 0, 0, 0 };
    D3D12_DRAW_ARGUMENTS* mappedArgs = nullptr;
    indirectArgsUploadBuffer_ = BufferManager::CreateMappedBuffer<D3D12_DRAW_ARGUMENTS>(device, 1, &mappedArgs);
    *mappedArgs = drawArgs;

    // 2. GPU内で全草データを保持するマスターバッファ (Default Heap + UAV/SRV)
    generatedGrassBuffer_ = BufferManager::CreateUAVBufferResource(
        device, sizeof(GrassInstanceData) * kMaxInstances);

    generatedSrvIndex_ = srvManager->CreateStructuredBufferSRV(
        generatedGrassBuffer_.Get(), kMaxInstances, sizeof(GrassInstanceData));
    generatedUavIndex_ = srvManager->CreateStructuredBufferUAV(
        generatedGrassBuffer_.Get(), kMaxInstances, sizeof(GrassInstanceData));

    // 草生成用パラメータの定数バッファ作成
    generationDataResource_ = BufferManager::CreateMappedConstantBuffer<GrassGenerationData>(
        device, &mappedGenData_);

    // 3. ダブルバッファリング用リソース & ビューの作成
    for (int i = 0; i < kFrameCount; ++i)
    {
        // Output Instance Buffer (Default Heap)
        outputInstanceBuffer_[i] = BufferManager::CreateUAVBufferResource(
            device, sizeof(GrassInstanceData) * kMaxInstances);

        // Indirect Draw Args Buffer (Default Heap)
        indirectArgsBuffer_[i] = BufferManager::CreateUAVBufferResource(
            device, sizeof(D3D12_DRAW_ARGUMENTS));

        // Constant Buffers
        materialResource_[i] = BufferManager::CreateMappedConstantBuffer<GrassMaterialData>(
            device, &mappedMaterial_[i]);

        cullingDataResource_[i] = BufferManager::CreateMappedConstantBuffer<GrassCullingData>(
            device, &mappedCullingData_[i]);

        // ビュー作成
        outputUavIndex_[i] = srvManager->CreateStructuredBufferUAV(
            outputInstanceBuffer_[i].Get(), kMaxInstances, sizeof(GrassInstanceData));

        outputSrvIndex_[i] = srvManager->CreateStructuredBufferSRV(
            outputInstanceBuffer_[i].Get(), kMaxInstances, sizeof(GrassInstanceData));

        indirectUavIndex_[i] = srvManager->CreateRawBufferUAV(
            indirectArgsBuffer_[i].Get(), sizeof(D3D12_DRAW_ARGUMENTS));
    }

    // 4. CullingCS用 ディスクリプタヒープ
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 16 * kFrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cullingHeap_));

    // 5. ExecuteIndirect用 コマンドシグネチャ作成
    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

    D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc = {};
    cmdSigDesc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
    cmdSigDesc.NumArgumentDescs = 1;
    cmdSigDesc.pArgumentDescs = &argDesc;

    device->CreateCommandSignature(&cmdSigDesc, nullptr, IID_PPV_ARGS(&commandSignature_));
}

void GrassRenderer::BeginFrame()
{
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

// ★ GPU上でGrassGenerationCSを実行し、草データを全自動生成する関数
void GrassRenderer::GenerateGrass(
    const RenderEnvironment& env,
    const GrassGenerationData& genData,
    uint32_t heightMapSrvHandle,
    uint32_t densityMapSrvHandle,
    D3D12_GPU_VIRTUAL_ADDRESS terrainSettingsAddress)
{
    auto* cmdList = env.commandManager->GetCommandList();

    // 1. 生成パラメータの更新
    memcpy(mappedGenData_, &genData, sizeof(GrassGenerationData));

    // 2. バッファを UAV ステートへ遷移
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        generatedGrassBuffer_.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(1, &barrier);

    // 3. Generation CS のセットアップ
    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("GrassGenerationCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("GrassGenerationCS"));

    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetComputeRootConstantBufferView(0, generationDataResource_->GetGPUVirtualAddress()); // b0: GrassGenerationData
    cmdList->SetComputeRootConstantBufferView(1, terrainSettingsAddress);

    cmdList->SetComputeRootDescriptorTable(2, env.srvManager->GetSRVHandleGPU(heightMapSrvHandle)); // t0: HeightMap
    cmdList->SetComputeRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(densityMapSrvHandle)); // t1: DensityMap
    cmdList->SetComputeRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(generatedUavIndex_)); // u0: OutputGrass

    // Dispatch 実行
    UINT totalGroups = (genData.maxGrassPerChunk + 63) / 64;
    UINT groupX = 1024; 
    UINT groupY = (totalGroups + groupX - 1) / groupX;
    cmdList->Dispatch(groupX, groupY, 1);

    // 4. バッファを SRV ステートに戻す（CullingCS読み込み用）
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        generatedGrassBuffer_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);

    totalGeneratedCount_ = genData.maxGrassPerChunk;
}

void GrassRenderer::Draw(
    const RenderEnvironment& env,
    uint32_t windMapTextureHandle,
    ShadowMap* shadowMap,
    const GrassMaterialData& materialData,
    const GrassCullingData& cullingData)
{
    if (totalGeneratedCount_ == 0) return;

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12Device* device = env.device->GetDevice();

    // ==========================================================
    // 1. 定数バッファのコピー
    // ==========================================================
    memcpy(mappedMaterial_[currentFrameIndex_], &materialData, sizeof(GrassMaterialData));

    GrassCullingData actualCullingData = cullingData;
    actualCullingData.totalInstanceCount = totalGeneratedCount_;
    memcpy(mappedCullingData_[currentFrameIndex_], &actualCullingData, sizeof(GrassCullingData));

    // ==========================================================
    // 2. 間接描画引数バッファのリセット (InstanceCount を 0 にリセット)
    // ==========================================================
    D3D12_RESOURCE_BARRIER resetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        indirectArgsBuffer_[currentFrameIndex_].Get(),
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &resetBarrier);

    cmdList->CopyBufferRegion(
        indirectArgsBuffer_[currentFrameIndex_].Get(), 0,
        indirectArgsUploadBuffer_.Get(), 0,
        sizeof(D3D12_DRAW_ARGUMENTS));

    // ==========================================================
    // 3. GrassCullingCS (GPUカリング処理)
    // ==========================================================
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

    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("GrassCullingCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("GrassCullingCS"));

    ID3D12DescriptorHeap* heaps[] = { cullingHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // コピー先のインデックスをフレームごとにずらす (1フレームあたり3つ使用するので 3 * currentFrameIndex_)
    UINT destOffset = 3 * currentFrameIndex_;

    D3D12_CPU_DESCRIPTOR_HANDLE destCPU = cullingHeap_->GetCPUDescriptorHandleForHeapStart();
    destCPU.ptr += destOffset * handleSize;

    D3D12_GPU_DESCRIPTOR_HANDLE destGPU = cullingHeap_->GetGPUDescriptorHandleForHeapStart();
    destGPU.ptr += destOffset * handleSize;

    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 0, handleSize), env.srvManager->GetSRVHandleCPU_ForCopying(generatedSrvIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 1, handleSize), env.srvManager->GetSRVHandleCPU_ForCopying(outputUavIndex_[currentFrameIndex_]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 2, handleSize), env.srvManager->GetSRVHandleCPU_ForCopying(indirectUavIndex_[currentFrameIndex_]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cmdList->SetComputeRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress()); 
    cmdList->SetComputeRootConstantBufferView(1, cullingDataResource_[currentFrameIndex_]->GetGPUVirtualAddress()); 
    cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 0, handleSize)); 
    cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 1, handleSize)); 
    cmdList->SetComputeRootDescriptorTable(4, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 2, handleSize)); 

    UINT totalGroups = (totalGeneratedCount_ + 63) / 64;
    UINT groupX = 1024;
    UINT groupY = (totalGroups + groupX - 1) / groupX;
    cmdList->Dispatch(groupX, groupY, 1);

    // ==========================================================
    // 4. 実際の描画 (ExecuteIndirect)
    // ==========================================================
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

    cmdList->SetPipelineState(env.psoManager->GetPSO("Grass"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Grass"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    ID3D12DescriptorHeap* mainHeaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, mainHeaps);

    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(2, materialResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, cullingDataResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());

    // CSが出力した Output Buffer を VS の SRV として設定
    cmdList->SetGraphicsRootShaderResourceView(5, outputInstanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(windMapTextureHandle));
    if (shadowMap)
    {
        cmdList->SetGraphicsRootDescriptorTable(7, shadowMap->GetSRVHandle());
    }

    cmdList->ExecuteIndirect(
        commandSignature_.Get(),
        1,
        indirectArgsBuffer_[currentFrameIndex_].Get(),
        0,
        nullptr,
        0);
}

}