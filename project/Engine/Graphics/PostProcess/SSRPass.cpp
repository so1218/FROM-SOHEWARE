#include "pch.h"
#include "SSRPass.h"
#include "Engine.h"

void SSRPass::Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager)
{
    InitializeBase(engine, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
    psoManager_ = psoManager;

    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();

    // 定数バッファ
    cbSSR_ = BufferManager::CreateBufferResource(device, sizeof(SSRSettings));
    cbSSR_->Map(0, nullptr, reinterpret_cast<void**>(&ssaoData_));
    *ssaoData_ = SSRSettings();

    ssaoData_->maxDistance = 50.0f;  
    ssaoData_->stepSize = 0.1f;  
    ssaoData_->maxSteps = 128; 
    ssaoData_->thickness = 0.1f;

    // 入力用DescriptorHeap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 4;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));
}

void SSRPass::Execute(
    ID3D12GraphicsCommandList* cmdList,
    D3D12_CPU_DESCRIPTOR_HANDLE sceneCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE normalCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE depthCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE materialCPU)
{
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle = passHeap_->GetCPUDescriptorHandleForHeapStart();
    UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Descriptorを専用ヒープにコピー
    device->CopyDescriptorsSimple(1, destHandle, sceneCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    destHandle.ptr += size;
    device->CopyDescriptorsSimple(1, destHandle, normalCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    destHandle.ptr += size;
    device->CopyDescriptorsSimple(1, destHandle, depthCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    destHandle.ptr += size;
    device->CopyDescriptorsSimple(1, destHandle, materialCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    PreDraw(cmdList);

    cmdList->SetGraphicsRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("SSR"));
    cmdList->SetPipelineState(psoManager_->GetPSO("SSR"));

    // Heap設定
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 定数バッファ
    cmdList->SetGraphicsRootConstantBufferView(0, cbSSR_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());

    // テクスチャテーブル 
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = passHeap_->GetGPUDescriptorHandleForHeapStart();

    cmdList->SetGraphicsRootDescriptorTable(2, gpuHandle); 
    gpuHandle.ptr += size;
    cmdList->SetGraphicsRootDescriptorTable(3, gpuHandle); 
    gpuHandle.ptr += size;
    cmdList->SetGraphicsRootDescriptorTable(4, gpuHandle); 
    gpuHandle.ptr += size;
    cmdList->SetGraphicsRootDescriptorTable(5, gpuHandle); 

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}