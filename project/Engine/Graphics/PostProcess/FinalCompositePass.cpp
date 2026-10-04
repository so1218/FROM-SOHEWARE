#include "pch.h"
#include "FinalCompositePass.h"
#include "BufferManager.h"
#include "TimeManager.h"
#include "Engine.h"
#include "PIXColors.h"

namespace FE
{

void FinalCompositePass::Initialize(
    Engine* engine,
    uint32_t w,
    uint32_t h,
    PSOManager* pso,
    SRVManager* srvManager)
{
    InitializeBase(engine, w, h);
    psoManager_ = pso;
    srvManager_ = srvManager;

    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();

    // 最終合成用定数バッファ
    constantBuffer_ = BufferManager::CreateMappedConstantBuffer<FinalCompositeSettings>(
        device,
        &cbData_
    );

    cbData_->bloomIntensity = 0.8f;
    cbData_->enableSSAO = false;
    cbData_->enableDoF = false;
    cbData_->enableSSR = false;  
    cbData_->ssrIntensity = 1.0f;
    cbData_->enableVolumetricFog = false;

    // 入力テクスチャ用 SRV ヒープ
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 7;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(
        &heapDesc,
        IID_PPV_ARGS(&passHeap_)
    );

    passHeap_->SetName(L"FinalCompositePass_Heap");

    descriptorSize_ =
        device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );
}

void FinalCompositePass::SetupInputViews(
    ID3D12Device* device,
    D3D12_CPU_DESCRIPTOR_HANDLE sceneCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE bloomCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE dofCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE depthCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE volumetricFogSRV,
    D3D12_CPU_DESCRIPTOR_HANDLE ssaoSRV
)
{
    // 専用ヒープの先頭
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle =
        passHeap_->GetCPUDescriptorHandleForHeapStart();

    uint32_t descriptorSize =
        device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );

    // Scene
    device->CopyDescriptorsSimple(
        1, destHandle, sceneCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );
    destHandle.ptr += descriptorSize;

    // Bloom
    device->CopyDescriptorsSimple(
        1, destHandle, bloomCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );
    destHandle.ptr += descriptorSize;

    // DoF
    device->CopyDescriptorsSimple(
        1, destHandle, dofCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );
    destHandle.ptr += descriptorSize;

    // Depth
    device->CopyDescriptorsSimple(
        1, destHandle, depthCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );
    destHandle.ptr += descriptorSize;

    // GodRay
    device->CopyDescriptorsSimple(1, destHandle, volumetricFogSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    destHandle.ptr += descriptorSize;

    // SSAO
    device->CopyDescriptorsSimple(1, destHandle, ssaoSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    destHandle.ptr += descriptorSize; 
}

void FinalCompositePass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
    D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    PIXScopedEvent(cmdList, PIXColors::PostProcess, "Final Composite Pass");

    PreDraw(cmdList);

    // パイプライン
    cmdList->SetPipelineState(
        psoManager_->GetPSO("FinalComposite")
    );

    // ディスクリプタヒープ
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 合成設定
    cmdList->SetGraphicsRootConstantBufferView(
        0, constantBuffer_->GetGPUVirtualAddress()
    );

    // カメラ定数
    cmdList->SetGraphicsRootConstantBufferView(
        1,
        engine_->GetGlobalConstants()
        ->GetResource()
        ->GetGPUVirtualAddress()
    );

    // 入力テクスチャ
    cmdList->SetGraphicsRootDescriptorTable(
        2,
        passHeap_->GetGPUDescriptorHandleForHeapStart()
    );

    // フルスクリーン描画
    cmdList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}

}