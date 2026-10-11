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
#include "EnvironmentManager.h"
#include "PIXColors.h"

namespace FE
{

void GrassRenderer::Initialize(const RenderEnvironment& env)
{
    ID3D12Device* device = env.device->GetDevice();
    auto* srvManager = env.srvManager;

    // GPU-Driven Rendering 用リソース
    // ExecuteIndirectの引数初期値 (頂点数8=ビルボード1枚分。InstanceCountはカリングCS側で動的決定)
    D3D12_DRAW_ARGUMENTS drawArgs = { 8, 0, 0, 0 };
    D3D12_DRAW_ARGUMENTS* mappedArgs = nullptr;
    indirectArgsUploadBuffer_ = BufferManager::CreateMappedBuffer<D3D12_DRAW_ARGUMENTS>(device, 1, &mappedArgs);
    *mappedArgs = drawArgs;

    // GPU上でプロシージャル生成した全草データを保持するバッファ
    generatedGrassBuffer_ = BufferManager::CreateUAVBufferResource(
        device, sizeof(GrassInstanceData) * kMaxInstances);

    generatedSrvIndex_ = srvManager->CreateStructuredBufferSRV(
        generatedGrassBuffer_.Get(), kMaxInstances, sizeof(GrassInstanceData));
    generatedUavIndex_ = srvManager->CreateStructuredBufferUAV(
        generatedGrassBuffer_.Get(), kMaxInstances, sizeof(GrassInstanceData));

    generationDataResource_ = BufferManager::CreateMappedConstantBuffer<GrassGenerationData>(
        device, &mappedGenData_);

    // フレームリソース (Double Buffering)
    for (int i = 0; i < kFrameCount; ++i)
    {
        // カリングを通過した可視インスタンスのみを格納するバッファ
        outputInstanceBuffer_[i] = BufferManager::CreateUAVBufferResource(
            device, sizeof(GrassInstanceData) * kMaxInstances);

        // カリングCS内で有効数をアトミック加算して書き込む引数バッファ
        indirectArgsBuffer_[i] = BufferManager::CreateUAVBufferResource(
            device, sizeof(D3D12_DRAW_ARGUMENTS));

        materialResource_[i] = BufferManager::CreateMappedConstantBuffer<GrassMaterialData>(
            device, &mappedMaterial_[i]);
        cullingDataResource_[i] = BufferManager::CreateMappedConstantBuffer<GrassCullingData>(
            device, &mappedCullingData_[i]);

        outputUavIndex_[i] = srvManager->CreateStructuredBufferUAV(
            outputInstanceBuffer_[i].Get(), kMaxInstances, sizeof(GrassInstanceData));
        outputSrvIndex_[i] = srvManager->CreateStructuredBufferSRV(
            outputInstanceBuffer_[i].Get(), kMaxInstances, sizeof(GrassInstanceData));
        indirectUavIndex_[i] = srvManager->CreateRawBufferUAV(
            indirectArgsBuffer_[i].Get(), sizeof(D3D12_DRAW_ARGUMENTS));
    }

    // CullingCS用 ローカルディスクリプタヒープ
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 16 * kFrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cullingHeap_));

    // ExecuteIndirect用 コマンドシグネチャ (DrawInstanced用)
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

void GrassRenderer::GenerateGrass(
    const RenderEnvironment& env,
    const GrassGenerationData& genData,
    uint32_t heightMapSrvHandle,
    uint32_t densityMapSrvHandle,
    D3D12_GPU_VIRTUAL_ADDRESS terrainSettingsAddress)
{
    auto* cmdList = env.commandManager->GetCommandList();

    memcpy(mappedGenData_, &genData, sizeof(GrassGenerationData));

    // 生成先バッファを UAV ステートへ遷移
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        generatedGrassBuffer_.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(1, &barrier);

    // Grass Generation Compute Shader
    // ハイトマップ・密度マップを参照し、GPU上で草のインスタンスデータをプロシージャルに事前生成
    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("GrassGenerationCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("GrassGenerationCS"));

    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetComputeRootConstantBufferView(0, generationDataResource_->GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(1, terrainSettingsAddress);

    cmdList->SetComputeRootDescriptorTable(2, env.srvManager->GetSRVHandleGPU(heightMapSrvHandle));
    cmdList->SetComputeRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(densityMapSrvHandle));
    cmdList->SetComputeRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(generatedUavIndex_));

    // スレッドグループの算出 (1グループ = 64スレッド)
    uint32_t totalGroups = (genData.maxGrassPerChunk + 63) / 64;
    uint32_t groupX = 1024;
    uint32_t groupY = (totalGroups + groupX - 1) / groupX;
    cmdList->Dispatch(groupX, groupY, 1);

    // 次のカリングフェーズで読み込むため SRV ステートへ遷移
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
    const GrassCullingData& cullingData,
    D3D12_GPU_VIRTUAL_ADDRESS interactionCBAddress,
    D3D12_GPU_DESCRIPTOR_HANDLE interactionSrvHandle)
{
    // 草が生成されていない場合は処理をスキップ
    if (totalGeneratedCount_ == 0)
    {
        return;
    }

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12Device* device = env.device->GetDevice();

    PIXScopedEvent(cmdList, FE::PIXColors::Geometry, "Grass Pass (Total Generated: %u)", totalGeneratedCount_);

    // 描画およびカリングパラメータの更新
    memcpy(mappedMaterial_[currentFrameIndex_], &materialData, sizeof(GrassMaterialData));

    GrassCullingData actualCullingData = cullingData;
    actualCullingData.totalInstanceCount = totalGeneratedCount_;
    memcpy(mappedCullingData_[currentFrameIndex_], &actualCullingData, sizeof(GrassCullingData));

    // カリング実行前に間接描画のインスタンス数をゼロに初期化
    D3D12_RESOURCE_BARRIER resetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        indirectArgsBuffer_[currentFrameIndex_].Get(),
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &resetBarrier);

    cmdList->CopyBufferRegion(
        indirectArgsBuffer_[currentFrameIndex_].Get(), 0,
        indirectArgsUploadBuffer_.Get(), 0,
        sizeof(D3D12_DRAW_ARGUMENTS));

    // コンピュートシェーダーによるGPU駆動カリング
    // 全草データから可視インスタンスのみを抽出し、間接描画バッファのカウンターを加算
    {
        PIXScopedEvent(cmdList, FE::PIXColors::Compute, "Grass Culling CS");

        // 全草データから可視インスタンスのみを抽出し、間接描画バッファのカウンターを加算
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

        // フレーム毎にディスクリプタの書き込み位置をずらし、GPU実行中のリソース競合を防止
        uint32_t handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        uint32_t destOffset = 3 * currentFrameIndex_;

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

        uint32_t totalGroups = (totalGeneratedCount_ + 63) / 64;
        uint32_t groupX = 1024;
        uint32_t groupY = (totalGroups + groupX - 1) / groupX;
        cmdList->Dispatch(groupX, groupY, 1);
    }

    // カリングを通過した可視インスタンスの一括描画
    {
        PIXScopedEvent(cmdList, FE::PIXColors::Geometry, "Grass ExecuteIndirect Draw");

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
        cmdList->SetGraphicsRootConstantBufferView(4, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(5, interactionCBAddress);

        if (shadowMap)
        {
            cmdList->SetGraphicsRootConstantBufferView(6, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootDescriptorTable(9, shadowMap->GetSRVHandle());
        }

        cmdList->SetGraphicsRootShaderResourceView(7, outputInstanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(windMapTextureHandle));
        cmdList->SetGraphicsRootDescriptorTable(10, interactionSrvHandle);

        cmdList->ExecuteIndirect(commandSignature_.Get(), 1, indirectArgsBuffer_[currentFrameIndex_].Get(), 0, nullptr, 0);
    
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }
}

}