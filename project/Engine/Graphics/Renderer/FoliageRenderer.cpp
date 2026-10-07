#include "pch.h"
#include "FoliageRenderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"
#include "EnvironmentManager.h"
#include "PIXColors.h"

namespace FE
{

void FoliageRenderer::Initialize(const RenderEnvironment& env, const std::vector<FoliageTypeConfig>& configs)
{
    if (configs.empty()) return;

    isGenerated_ = false;

    ID3D12Device* device = env.device->GetDevice();
    auto* srvManager = env.srvManager;
    size_t numTypes = configs.size();

    types_.resize(numTypes);

    uint32_t* counterResetMapped = nullptr;
    counterResetUploadBuffer_ = BufferManager::CreateMappedBuffer<uint32_t>(device, 1, &counterResetMapped);
    *counterResetMapped = 0;

    // カリングCSのバインドに必要なディスクリプタを連続領域として確保
    // レイアウト: [t0: Input, t1: Counter, u0: Output, u1: IndirectArgs] * 種類数 * フレーム数
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 4 * static_cast<uint32_t>(numTypes) * kFrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cullingHeap_));

    uint32_t handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE destCPU = cullingHeap_->GetCPUDescriptorHandleForHeapStart();

    for (int i = 0; i < kFrameCount; ++i)
    {
        cullingDataResource_[i] = BufferManager::CreateMappedConstantBuffer<FoliageCullingData>(device, &mappedCullingData_[i]);

        for (size_t typeIdx = 0; typeIdx < numTypes; ++typeIdx)
        {
            auto& res = types_[typeIdx];
            if (i == 0) {
                res.config = configs[typeIdx];
            }

            res.generationDataResource[i] = BufferManager::CreateMappedConstantBuffer<FoliageGenerationData>(device, &res.mappedGenData[i]);
            res.materialResource[i] = BufferManager::CreateMappedConstantBuffer<FoliageMaterialData>(device, &res.mappedMaterial[i]);

            // Generationパスのリソースは更新不要なため、初回フレームでのみ構築
            if (i == 0)
            {
                res.generatedBuffer = BufferManager::CreateUAVBufferResource(device, sizeof(FoliageInstanceData) * kMaxInstances);
                res.appendCounterBuffer = BufferManager::CreateUAVBufferResource(device, sizeof(uint32_t));

                res.generatedSrvIndex = srvManager->CreateStructuredBufferSRV(res.generatedBuffer.Get(), kMaxInstances, sizeof(FoliageInstanceData));
                res.generatedUavIndex = srvManager->CreateAppendStructuredBufferUAV(
                    res.generatedBuffer.Get(), res.appendCounterBuffer.Get(), kMaxInstances, sizeof(FoliageInstanceData));

                // インスタンスの総数をCS側で間接参照するためのRawBufferビュー
                res.counterSrvIndex = srvManager->CreateRawBufferSRV(res.appendCounterBuffer.Get(), sizeof(uint32_t));
            }

            res.outputInstanceBuffer[i] = BufferManager::CreateUAVBufferResource(device, sizeof(FoliageInstanceData) * kMaxInstances);
            res.indirectArgsBuffer[i] = BufferManager::CreateUAVBufferResource(device, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

            res.indirectArgsUploadBuffer[i] = BufferManager::CreateMappedBuffer<D3D12_DRAW_INDEXED_ARGUMENTS>(device, 1, &res.mappedArgs[i]);
            *res.mappedArgs[i] = {};

            res.outputUavIndex[i] = srvManager->CreateStructuredBufferUAV(res.outputInstanceBuffer[i].Get(), kMaxInstances, sizeof(FoliageInstanceData));
            res.indirectUavIndex[i] = srvManager->CreateRawBufferUAV(res.indirectArgsBuffer[i].Get(), sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

            // 実行時の動的コピーのオーバーヘッドを避けるため、初期化時にヒープへ事前コピー
            device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(res.generatedSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            destCPU.ptr += handleSize;

            device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(res.counterSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            destCPU.ptr += handleSize;

            device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(res.outputUavIndex[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            destCPU.ptr += handleSize;

            device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(res.indirectUavIndex[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            destCPU.ptr += handleSize;
        }
    }

    // GPUドリブンレンダリングの要となるCommandSignatureの設定
    // CSが間接引数バッファに書き込んだIndex/InstanceCountを用いてDrawIndexedを発行
    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
    D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc = {};
    cmdSigDesc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
    cmdSigDesc.NumArgumentDescs = 1;
    cmdSigDesc.pArgumentDescs = &argDesc;
    device->CreateCommandSignature(&cmdSigDesc, nullptr, IID_PPV_ARGS(&commandSignature_));
}

void FoliageRenderer::BeginFrame()
{
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void FoliageRenderer::GenerateFoliage(
    const RenderEnvironment& env,
    uint32_t heightMapSrvHandle,
    D3D12_GPU_VIRTUAL_ADDRESS terrainSettingsAddress,
    uint32_t terrainWidth, uint32_t terrainDepth)
{
    if (types_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    size_t numTypes = types_.size();

    // 初回生成時と再生成時でリソースステートが異なるため、遷移元を動的に解決
    D3D12_RESOURCE_STATES beforeState = isGenerated_ ?
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE :
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    barriers.reserve(numTypes * 2);

    for (size_t i = 0; i < numTypes; ++i) {
        if (isGenerated_) {
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                types_[i].generatedBuffer.Get(), beforeState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        }
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
            types_[i].appendCounterBuffer.Get(), beforeState, D3D12_RESOURCE_STATE_COPY_DEST));
    }

    if (!barriers.empty()) {
        cmdList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
    }

    // AppendStructuredBufferのカウンタをCPUをストールさせずにGPU上でゼロクリア
    for (size_t i = 0; i < numTypes; ++i) {
        cmdList->CopyBufferRegion(types_[i].appendCounterBuffer.Get(), 0, counterResetUploadBuffer_.Get(), 0, sizeof(uint32_t));
    }

    std::vector<D3D12_RESOURCE_BARRIER> counterBarriers2(numTypes);
    for (size_t i = 0; i < numTypes; ++i) {
        counterBarriers2[i] = CD3DX12_RESOURCE_BARRIER::Transition(
            types_[i].appendCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }
    cmdList->ResourceBarrier(static_cast<uint32_t>(numTypes), counterBarriers2.data());

    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("FoliageGenerationCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("FoliageGenerationCS"));

    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetComputeRootConstantBufferView(1, terrainSettingsAddress);
    cmdList->SetComputeRootDescriptorTable(2, env.srvManager->GetSRVHandleGPU(heightMapSrvHandle));

    uint32_t dispatchX = (terrainWidth + 7) / 8;
    uint32_t dispatchY = (terrainDepth + 7) / 8;

    // 種類ごとに固有のDensityMapとパラメータをバインドし、地形上にインスタンスを動的生成
    for (size_t i = 0; i < numTypes; ++i)
    {
        auto& res = types_[i];
        memcpy(res.mappedGenData[currentFrameIndex_], &res.config.genData, sizeof(FoliageGenerationData));

        cmdList->SetComputeRootConstantBufferView(0, res.generationDataResource[currentFrameIndex_]->GetGPUVirtualAddress());
        cmdList->SetComputeRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(res.config.densityMapSrvHandle));
        cmdList->SetComputeRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(res.generatedUavIndex));

        cmdList->Dispatch(dispatchX, dispatchY, 1);
    }

    // 後段のカリングパスでの読み取りに備えてSRVへ遷移
    std::vector<D3D12_RESOURCE_BARRIER> readBarriers(numTypes * 2);
    for (size_t i = 0; i < numTypes; ++i) {
        readBarriers[i * 2 + 0] = CD3DX12_RESOURCE_BARRIER::Transition(
            types_[i].generatedBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        readBarriers[i * 2 + 1] = CD3DX12_RESOURCE_BARRIER::Transition(
            types_[i].appendCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    cmdList->ResourceBarrier(static_cast<uint32_t>(readBarriers.size()), readBarriers.data());

    isGenerated_ = true;
}


void FoliageRenderer::Draw(
    const RenderEnvironment& env,
    ShadowMap* shadowMap,
    const FoliageCullingData& cullingData,
    D3D12_GPU_VIRTUAL_ADDRESS interactionCBAddress,
    D3D12_GPU_DESCRIPTOR_HANDLE interactionSrvHandle)
{
    if (types_.empty() || !mappedCullingData_[currentFrameIndex_])
    {
        return;
    }

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12Device* device = env.device->GetDevice();
    size_t numTypes = types_.size();

    PIXScopedEvent(cmdList, FE::PIXColors::Geometry, "Foliage Pass (%zu Types)", numTypes);

    memcpy(mappedCullingData_[currentFrameIndex_], &cullingData, sizeof(FoliageCullingData));
    uint32_t handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // -----------------------------------------------------------
    // Pass 1: GPUカリング
    // 前段で生成したインスタンス群に対し、視錐台/距離カリングを適用
    // -----------------------------------------------------------
    {
        PIXScopedEvent(cmdList, FE::PIXColors::Compute, "Foliage GPU Culling CS Pass");

        cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("FoliageCullingCS"));
        cmdList->SetPipelineState(env.psoManager->GetPSO("FoliageCullingCS"));

        ID3D12DescriptorHeap* cullingHeaps[] = { cullingHeap_.Get() };
        cmdList->SetDescriptorHeaps(1, cullingHeaps);

        for (int typeIdx = 0; typeIdx < numTypes; ++typeIdx)
        {
            // タイプごとのカリング処理用サブスコープ
            PIXScopedEvent(cmdList, FE::PIXColors::Compute, "Culling CS [Type %d]", typeIdx);

            auto& res = types_[typeIdx];
            memcpy(res.mappedMaterial[currentFrameIndex_], &res.config.material, sizeof(FoliageMaterialData));

            // ExecuteIndirect用の引数バッファ初期化。InstanceCountは後段のCS内で可視判定に通過した数だけインクリメントされる
            D3D12_DRAW_INDEXED_ARGUMENTS drawArgs = {};
            drawArgs.IndexCountPerInstance = static_cast<uint32_t>(res.config.mesh->GetIndexCount());
            drawArgs.InstanceCount = 0;
            *res.mappedArgs[currentFrameIndex_] = drawArgs;

            D3D12_RESOURCE_BARRIER resetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
                res.indirectArgsBuffer[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
            cmdList->ResourceBarrier(1, &resetBarrier);

            cmdList->CopyBufferRegion(
                res.indirectArgsBuffer[currentFrameIndex_].Get(), 0,
                res.indirectArgsUploadBuffer[currentFrameIndex_].Get(), 0,
                sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

            D3D12_RESOURCE_BARRIER csBarriers[2] = {
                CD3DX12_RESOURCE_BARRIER::Transition(res.outputInstanceBuffer[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(res.indirectArgsBuffer[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            };
            cmdList->ResourceBarrier(2, csBarriers);

            // カリング専用に事前構築したDescriptorHeap上の連続領域を直接参照し、動的なバインドコストを回避
            uint32_t slotOffset = static_cast<uint32_t>((currentFrameIndex_ * numTypes + typeIdx) * 4);
            D3D12_GPU_DESCRIPTOR_HANDLE destGPU = cullingHeap_->GetGPUDescriptorHandleForHeapStart();
            destGPU.ptr += slotOffset * handleSize;

            cmdList->SetComputeRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
            cmdList->SetComputeRootConstantBufferView(1, cullingDataResource_[currentFrameIndex_]->GetGPUVirtualAddress());

            cmdList->SetComputeRootDescriptorTable(2, destGPU); destGPU.ptr += handleSize;
            cmdList->SetComputeRootDescriptorTable(3, destGPU); destGPU.ptr += handleSize;
            cmdList->SetComputeRootDescriptorTable(4, destGPU); destGPU.ptr += handleSize;
            cmdList->SetComputeRootDescriptorTable(5, destGPU);

            // 巨大な地形でインスタンス数が超過した場合の安全策としてDispatchの上限をクリップ
            uint32_t maxGroupsX = 1024;
            uint32_t dispatchX = std::min((uint32_t)(kMaxInstances + 63) / 64, maxGroupsX);
            uint32_t dispatchY = ((uint32_t)kMaxInstances + 65535) / 65536;
            cmdList->Dispatch(dispatchX, dispatchY, 1);
        }
    }

    // -----------------------------------------------------------
    // Pass 2: 間接描画
    // カリング済みのバッファを参照し、CPUを介さずにGPU上で描画コマンドを発行
    // -----------------------------------------------------------
    {
        PIXScopedEvent(cmdList, FE::PIXColors::Geometry, "Foliage Indirect Draw Pass");

        cmdList->SetPipelineState(env.psoManager->GetPSO("Foliage"));
        cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Foliage"));
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D12DescriptorHeap* mainHeaps[] = { env.srvManager->GetSRVHeap() };
        cmdList->SetDescriptorHeaps(1, mainHeaps);

        cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(2, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(4, interactionCBAddress);

        if (shadowMap)
        {
            cmdList->SetGraphicsRootConstantBufferView(5, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootDescriptorTable(8, shadowMap->GetSRVHandle());
        }

        cmdList->SetGraphicsRootDescriptorTable(9, interactionSrvHandle);

        for (int typeIdx = 0; typeIdx < numTypes; ++typeIdx)
        {
            // タイプごとの描画処理用サブスコープ
            PIXScopedEvent(cmdList, FE::PIXColors::Geometry, "ExecuteIndirect [Type %d]", typeIdx);

            auto& res = types_[typeIdx];

            D3D12_RESOURCE_BARRIER drawBarriers[2] = {
                CD3DX12_RESOURCE_BARRIER::Transition(res.outputInstanceBuffer[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(res.indirectArgsBuffer[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
            };
            cmdList->ResourceBarrier(2, drawBarriers);

            cmdList->SetGraphicsRootConstantBufferView(3, res.materialResource[currentFrameIndex_]->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootShaderResourceView(6, res.outputInstanceBuffer[currentFrameIndex_]->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(res.config.albedoSrvHandle));

            cmdList->IASetVertexBuffers(0, 1, &res.config.mesh->GetVertexBufferView());
            cmdList->IASetIndexBuffer(&res.config.mesh->GetIndexBufferView());

            cmdList->ExecuteIndirect(
                commandSignature_.Get(), 1,
                res.indirectArgsBuffer[currentFrameIndex_].Get(), 0, nullptr, 0);

            // 次フレームのGenerateパスに向けたステート復元
            D3D12_RESOURCE_BARRIER restoreBarriers[2] = {
                CD3DX12_RESOURCE_BARRIER::Transition(res.generatedBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(res.appendCounterBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            };
            cmdList->ResourceBarrier(2, restoreBarriers);
        }
    }
}

void FoliageRenderer::UpdateConfigs(const std::vector<FoliageTypeConfig>& configs)
{
    // ツールからのホットリロード用途。VRAM上のバッファ再確保を避けつつパラメータのみを更新
    size_t count = std::min(types_.size(), configs.size());
    for (size_t i = 0; i < count; ++i)
    {
        types_[i].config = configs[i];
    }
}

}