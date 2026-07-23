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

    // ★追加: メモリの再確保(アロケーション)によるCPUスパイクを防ぐため、
    // 最初に最大数分のメモリを確保しておく
    instanceQueue_.reserve(kMaxInstances);

    UINT materialBufferSize = (sizeof(GrassMaterialData) + 255) & ~255;
    UINT cullingBufferSize = (sizeof(GrassCullingData) + 255) & ~255;
    UINT instanceBufferSize = sizeof(GrassInstanceData) * kMaxInstances;

    // 間接描画引数のリセット用初期データ { VertexCount, InstanceCount, StartVertex, StartInstance }
    // 今回は1枚の草あたり7(または8)頂点。初期インスタンス数は0。
    D3D12_DRAW_ARGUMENTS drawArgs = { 8, 0, 0, 0 };
    indirectArgsUploadBuffer_ = BufferManager::CreateBufferResource(device, sizeof(D3D12_DRAW_ARGUMENTS));
    void* mappedArgs = nullptr;
    indirectArgsUploadBuffer_->Map(0, nullptr, &mappedArgs);
    memcpy(mappedArgs, &drawArgs, sizeof(D3D12_DRAW_ARGUMENTS));
    indirectArgsUploadBuffer_->Unmap(0, nullptr);

    // バッファとビューの作成
    for (int i = 0; i < kFrameCount; ++i)
    {
        // 1. Input Buffer (CPU -> GPU, CS SRV)
        inputInstanceBuffer_[i] = BufferManager::CreateBufferResource(device, instanceBufferSize);
        inputInstanceBuffer_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedInputData_[i]));

        // 2. Output Buffer (CS UAV, VS SRV) Default Heap推奨(UAVなので)
        outputInstanceBuffer_[i] = BufferManager::CreateUAVBufferResource(device, instanceBufferSize); // ※適切なUAVバッファ生成関数を使用

        // 3. Indirect Args Buffer (CS UAV, ExecuteIndirect)
        indirectArgsBuffer_[i] = BufferManager::CreateUAVBufferResource(device, sizeof(D3D12_DRAW_ARGUMENTS));

        // 4. Constant Buffers
        materialResource_[i] = BufferManager::CreateBufferResource(device, materialBufferSize);
        materialResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedMaterial_[i]));

        cullingDataResource_[i] = BufferManager::CreateBufferResource(device, cullingBufferSize);
        cullingDataResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedCullingData_[i]));

        // --- ビュー(SRV/UAV)の作成 ---
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = kMaxInstances;
        srvDesc.Buffer.StructureByteStride = sizeof(GrassInstanceData);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = kMaxInstances;
        uavDesc.Buffer.StructureByteStride = sizeof(GrassInstanceData);

        // Input SRV
        inputSrvIndex_[i] = srvManager->Allocate();
        device->CreateShaderResourceView(inputInstanceBuffer_[i].Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(inputSrvIndex_[i]));

        // Output UAV & SRV
        outputUavIndex_[i] = srvManager->Allocate();
        device->CreateUnorderedAccessView(outputInstanceBuffer_[i].Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(outputUavIndex_[i]));
        outputSrvIndex_[i] = srvManager->Allocate();
        device->CreateShaderResourceView(outputInstanceBuffer_[i].Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(outputSrvIndex_[i]));

        // Indirect Args UAV (Raw Buffer / ByteAddressBuffer 扱い)
        D3D12_UNORDERED_ACCESS_VIEW_DESC indirectUavDesc = {};
        indirectUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        indirectUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        indirectUavDesc.Buffer.NumElements = sizeof(D3D12_DRAW_ARGUMENTS) / 4;
        indirectUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW; // ByteAddressBuffer用フラグ

        indirectUavIndex_[i] = srvManager->Allocate();
        device->CreateUnorderedAccessView(indirectArgsBuffer_[i].Get(), nullptr, &indirectUavDesc, srvManager->GetSRVHandleCPU_ForCopying(indirectUavIndex_[i]));
    }

    // ディスクリプタヒープの作成 (GrassCullingCS用)
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 16; // 必要な数に合わせて調整
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));
    passHeap_->SetName(L"GrassRenderer_Heap");

    // --- コマンドシグネチャの作成 (ExecuteIndirect用) ---
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

void GrassRenderer::Submit(const Vector3& position, float height, float rotationY, float width, uint32_t packedColor)
{
    if (instanceQueue_.size() >= kMaxInstances) return;

    GrassInstanceData data;
    data.posAndHeight = Vector4(position.x, position.y, position.z, height);

    float colorAsFloat;
    std::memcpy(&colorAsFloat, &packedColor, sizeof(float));

    data.rotWidthColor = Vector4(rotationY, width, colorAsFloat, 0.0f);

    instanceQueue_.push_back(data);

    // 追加した場合はバッファを更新する必要があるためフラグを立てる
    dirtyFrames_ = kFrameCount;
}

void GrassRenderer::Draw(const RenderEnvironment& env, uint32_t windMapTextureHandle, ShadowMap* shadowMap, const GrassMaterialData& materialData, const GrassCullingData& cullingData)
{
    if (instanceQueue_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12Device* device = env.device->GetDevice();
    uint32_t instanceCount = static_cast<uint32_t>(Math::MyMin((size_t)kMaxInstances, instanceQueue_.size()));

    // ==========================================================
    // 1. データの転送 (CPU -> GPU)
    // ==========================================================

    // ★ここが最も重い48MBのコピー。配置が更新された後の数フレームだけ実行する。
    if (dirtyFrames_ > 0)
    {
        memcpy(mappedInputData_[currentFrameIndex_], instanceQueue_.data(), sizeof(GrassInstanceData) * instanceCount);
        dirtyFrames_--;
    }

    // マテリアルやカメラ情報(CullingData)は毎フレーム変化するが、
    // 数十バイト程度なので毎フレームコピーしてもCPU負荷はほぼゼロ(一瞬)
    memcpy(mappedMaterial_[currentFrameIndex_], &materialData, sizeof(GrassMaterialData));

    GrassCullingData actualCullingData = cullingData;
    actualCullingData.totalInstanceCount = instanceCount;
    memcpy(mappedCullingData_[currentFrameIndex_], &actualCullingData, sizeof(GrassCullingData));

    // ==========================================================
    // [STEP 1] 間接描画引数バッファのリセット (InstanceCount を 0 に戻す)
    // ==========================================================
    D3D12_RESOURCE_BARRIER resetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        indirectArgsBuffer_[currentFrameIndex_].Get(),
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, // 前フレームの最後の状態
        D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &resetBarrier);

    cmdList->CopyBufferRegion(indirectArgsBuffer_[currentFrameIndex_].Get(), 0, indirectArgsUploadBuffer_.Get(), 0, sizeof(D3D12_DRAW_ARGUMENTS));

    // ==========================================================
    // [STEP 2] GrassCullingCS (GPU間引き処理)
    // ==========================================================
    PIXScopedEvent(cmdList, PIX_COLOR(50, 200, 50), "Grass Culling CS");

    // バッファをUAVステートへ遷移
    D3D12_RESOURCE_BARRIER csBarriers[2] = {};
    csBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(outputInstanceBuffer_[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    csBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(indirectArgsBuffer_[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(2, csBarriers);

    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("GrassCullingCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("GrassCullingCS"));

    // --- ディスクリプタのコピー（VolumetricFogと同じ要領） ---
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE destCPU = passHeap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE destGPU = passHeap_->GetGPUDescriptorHandleForHeapStart();

    // t0: InputGrassData
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 0, handleSize), env.srvManager->GetSRVHandleCPU_ForCopying(inputSrvIndex_[currentFrameIndex_]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // u0: OutputGrassData
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 1, handleSize), env.srvManager->GetSRVHandleCPU_ForCopying(outputUavIndex_[currentFrameIndex_]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // u1: IndirectArgs
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 2, handleSize), env.srvManager->GetSRVHandleCPU_ForCopying(indirectUavIndex_[currentFrameIndex_]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Root Parameter の設定 (RootSignatureの定義に従う)
    cmdList->SetComputeRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress()); // b0
    cmdList->SetComputeRootConstantBufferView(1, cullingDataResource_[currentFrameIndex_]->GetGPUVirtualAddress()); // b1
    cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 0, handleSize)); // t0
    cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 1, handleSize)); // u0
    cmdList->SetComputeRootDescriptorTable(4, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 2, handleSize)); // u1

    // スレッドグループのディスパッチ (numthreads(64,1,1) に合わせる)
    UINT dispatchCount = (instanceCount + 63) / 64;
    cmdList->Dispatch(dispatchCount, 1, 1);

    // ==========================================================
    // [STEP 3] 実際の描画 (ExecuteIndirect)
    // ==========================================================
    PIXScopedEvent(cmdList, PIX_COLOR(50, 200, 50), "Grass Draw Indirect");

    // UAVから、VS読み込み(SRV)と間接描画引数(INDIRECT_ARGUMENT)へ遷移
    D3D12_RESOURCE_BARRIER drawBarriers[2] = {};
    drawBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(outputInstanceBuffer_[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    drawBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(indirectArgsBuffer_[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
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

    // Inputではなく、CSが出力した Output Buffer をグラフィックスシェーダーのSRVとして設定する
    cmdList->SetGraphicsRootShaderResourceView(5, outputInstanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(windMapTextureHandle));
    if (shadowMap)
    {
        cmdList->SetGraphicsRootDescriptorTable(7, shadowMap->GetSRVHandle());
    }

    // ★従来の DrawInstanced ではなく ExecuteIndirect を使用する
    cmdList->ExecuteIndirect(
        commandSignature_.Get(),
        1,
        indirectArgsBuffer_[currentFrameIndex_].Get(),
        0,
        nullptr,
        0);
}

void GrassRenderer::ClearInstances()
{
    instanceQueue_.clear();
    // マルチバッファリングされている全フレーム分のバッファを
    // 更新する必要があるため、kFrameCount をセットする
    dirtyFrames_ = kFrameCount;
}

}