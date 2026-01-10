#include "BloomCombinePass.h"
#include "BufferManager.h"
#include "TimeManager.h"
#include "Engine.h"

void BloomCombinePass::Initialize(
    Engine* engine,
    UINT w,
    UINT h,
    PSOManager* pso,
    SRVManager* srvManager)
{
    InitializeBase(engine, w, h);
    psoManager_ = pso;
    srvManager_ = srvManager;

    ID3D12Device* device = engine->graphicsDevice_->GetDevice();

    // 合成用定数バッファ
    cb_ = BufferManager::CreateBufferResource(device, sizeof(CombineSettings));
    cb_->Map(0, nullptr, reinterpret_cast<void**>(&combineData_));

    combineData_->bloomIntensity = 0.8f;
    combineData_->focusDistance = 0.0f;
    combineData_->focusRange = 0.0f;

    combineData_->fogColor = Vector3(0.6f, 0.7f, 0.8f);
    combineData_->fogStart = 10.0f;
    combineData_->fogEnd = 50.0f;

    combineData_->enableDoF = false;
    combineData_->enableFog = false;

    combineData_->godRayIntensity = 1.0f;

    // 入力テクスチャ用 SRV ヒープ
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 5;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(
        &heapDesc,
        IID_PPV_ARGS(&passHeap_)
    );

    passHeap_->SetName(L"BloomCombinePass_Heap");

    descriptorSize_ =
        device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );
}

void BloomCombinePass::SetupInputViews(
    ID3D12Device* device,
    D3D12_CPU_DESCRIPTOR_HANDLE sceneCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE bloomCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE dofCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE depthCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE godRaySRV)
{
    // 専用ヒープの先頭
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle =
        passHeap_->GetCPUDescriptorHandleForHeapStart();

    UINT descriptorSize =
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
    device->CopyDescriptorsSimple(1, destHandle, godRaySRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void BloomCombinePass::Execute(
    ID3D12GraphicsCommandList* cmdList,
    D3D12_GPU_DESCRIPTOR_HANDLE)
{
    PreDraw(cmdList);

    // パイプライン
    cmdList->SetPipelineState(
        psoManager_->GetPSO("BloomCombine")
    );

    // ディスクリプタヒープ
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 合成設定
    cmdList->SetGraphicsRootConstantBufferView(
        0, cb_->GetGPUVirtualAddress()
    );

    // カメラ定数
    cmdList->SetGraphicsRootConstantBufferView(
        1,
        engine_->globalConstants_
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