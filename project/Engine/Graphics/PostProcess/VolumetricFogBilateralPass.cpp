#include "pch.h"
#include "VolumetricFogBilateralPass.h"
#include "Engine.h"

namespace FE
{

void VolumetricFogBilateralPass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso)
{
    // コンピュートシェーダー用として初期化 (isCompute = true)
    InitializeBase(engine, w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, true);
    psoManager_ = pso;

    // 定数バッファの作成とマッピング
    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    constantBuffer_ = BufferManager::CreateMappedConstantBuffer(
        device,
        &cbData_
    );

    // 初期値設定
    cbData_->blurRadius = 5;
    cbData_->spatialSigma = 2.0f;
    cbData_->depthSigma = 0.001f;

    // パス用SRV/UAVヒープ作成（RawFog, Depth, OutputUAV の 3つ分）
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 3;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));
    passHeap_->SetName(L"VolumetricFogBilateral_Heap");
}

void VolumetricFogBilateralPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context, D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // ディスクリプタの集約コピー
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle = passHeap_->GetCPUDescriptorHandleForHeapStart();

    // t0: RawFog (前パスの出力)
    D3D12_CPU_DESCRIPTOR_HANDLE rawFogHandle = engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(rawFogSrvIndex_);
    device->CopyDescriptorsSimple(1, destHandle, rawFogHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // t1: Depth
    destHandle.ptr += handleSize;
    device->CopyDescriptorsSimple(1, destHandle, context.srvManager->GetSRVHandleCPU_ForCopying(context.sceneDepthSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // u0: Output (自分自身のテクスチャのUAV)
    destHandle.ptr += handleSize;
    D3D12_CPU_DESCRIPTOR_HANDLE uavHandleCPU = engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(uavIndex_);
    device->CopyDescriptorsSimple(1, destHandle, uavHandleCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // リソースバリア (CS用の状態遷移)
    PreCompute(cmdList); // 自らの出力をUAVへ

    // Depth は前のパスの最後で PIXEL_SHADER_RESOURCE に戻されているので遷移が必要
    D3D12_RESOURCE_BARRIER depthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->GetOffscreenDepthResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    cmdList->ResourceBarrier(1, &depthBarrier);

    // Compute Pipeline 設定
    cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VolumetricFogBilateralCS"));
    cmdList->SetPipelineState(psoManager_->GetPSO("VolumetricFogBilateralCS"));

    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // ルートパラメータ設定
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = passHeap_->GetGPUDescriptorHandleForHeapStart();

    cmdList->SetComputeRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(1, gpuHandle);
    gpuHandle.ptr += handleSize;
    cmdList->SetComputeRootDescriptorTable(2, gpuHandle);

    gpuHandle.ptr += handleSize;
    cmdList->SetComputeRootDescriptorTable(3, gpuHandle);

    // Dispatch 実行
    UINT dispatchX = (static_cast<UINT>(viewport_.Width) + 7) / 8;
    UINT dispatchY = (static_cast<UINT>(viewport_.Height) + 7) / 8;
    cmdList->Dispatch(dispatchX, dispatchY, 1);

    depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &depthBarrier);

    PostCompute(cmdList);
}

}