#include "BloomCombinePass.h"
#include "BufferManager.h"
#include "TimeManager.h"
#include "Engine.h"

void BloomCombinePass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso, SRVManager* srvManager)
{
    InitializeBase(engine, w, h);
    psoManager_ = pso;
    srvManager_ = srvManager;

    ID3D12Device* device = engine->graphicsDevice_->GetDevice();

    // --- 1. CombineSettings (b0) ---
    cb_ = BufferManager::CreateBufferResource(device, sizeof(CombineSettings));
    cb_->Map(0, nullptr, reinterpret_cast<void**>(&combineData_));
    combineData_->bloomIntensity = 0.8f;

    // --- 専用SRVヒープ作成 ---
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 3;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvHeap_));

    descriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void BloomCombinePass::SetupInputViews(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE sceneCPU, D3D12_CPU_DESCRIPTOR_HANDLE blurCPU, D3D12_CPU_DESCRIPTOR_HANDLE depthCPU)
{
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle = srvHeap_->GetCPUDescriptorHandleForHeapStart();

    // t0: Scene
    device->CopyDescriptorsSimple(1, destHandle, sceneCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // t1: Blur
    destHandle.ptr += descriptorSize_;
    device->CopyDescriptorsSimple(1, destHandle, blurCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // t2: Depth
    destHandle.ptr += descriptorSize_;
    device->CopyDescriptorsSimple(1, destHandle, depthCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void BloomCombinePass::Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE /*unused*/)
{
    PreDraw(cmdList);

    cmdList->SetPipelineState(psoManager_->GetPSO("BloomCombine"));

    // 専用SRVヒープ
    ID3D12DescriptorHeap* heaps[] = { srvHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // --- Root Parameterの設定 ---
    // ※注意: ルートシグネチャの定義順に合わせてインデックスを変えてください
    // ここでは以下を想定しています：
    // [0] CBV: CombineSettings (b0)
    // [1] DescriptorTable: Textures (t0-t2)
    // [2] CBV: PostEffectData (b1)

// [0] CBV: CombineSettings (b0)
    // 現在のルートシグネチャ定義の Index 0 に対応
    cmdList->SetGraphicsRootConstantBufferView(0, cb_->GetGPUVirtualAddress());

    // [1] DescriptorTable: Textures (t0, t1)
    // 現在のルートシグネチャ定義の Index 1 に対応
    cmdList->SetGraphicsRootDescriptorTable(1, srvHeap_->GetGPUDescriptorHandleForHeapStart());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}