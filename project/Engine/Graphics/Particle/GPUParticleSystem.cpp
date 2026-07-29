#include "pch.h"
#include "GPUParticleSystem.h"

void GPUParticleSystem::Initialize(ID3D12Device* device)
{
    uint64_t freeListSize = sizeof(uint32_t) * kMaxParticles;
    D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };
    D3D12_RESOURCE_DESC uavDesc = {};
    uavDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uavDesc.Width = freeListSize;
    uavDesc.Height = 1;
    uavDesc.DepthOrArraySize = 1;
    uavDesc.MipLevels = 1;
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.SampleDesc.Count = 1;
    uavDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    uavDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &uavDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&freeListBuffer_));

    uavDesc.Width = sizeof(uint32_t);
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &uavDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&freeListCounter_));

}

void GPUParticleSystem::Init(ID3D12GraphicsCommandList* commandList)
{
    commandList->SetComputeRootSignature(computeRootSignature_);
    commandList->SetPipelineState(computePSO_Init_);

    // 10000 インデックス分 Dispatch (64スレッド/グループ)
    UINT groupCountX = (kMaxParticles + 63) / 64;
    commandList->Dispatch(groupCountX, 1, 1);

    // 初期化完了を担保するための UAV バリアを設定
    D3D12_RESOURCE_BARRIER uavBarriers[2] = {};
    uavBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarriers[0].UAV.pResource = freeListBuffer_.Get();
    uavBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarriers[1].UAV.pResource = freeListCounter_.Get();

    commandList->ResourceBarrier(2, uavBarriers);
}

void GPUParticleSystem::Emit(ID3D12GraphicsCommandList* commandList, uint32_t emitCount, DirectX::XMFLOAT3 emitterPos)
{
    EmitterData cbData = {};
    cbData.emitterPos = emitterPos;
    cbData.time = totalTime_;
    cbData.emitCount = emitCount;

    void* mappedPtr = nullptr;
    emitterBuffer_->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, &cbData, sizeof(EmitterData));
    emitterBuffer_->Unmap(0, nullptr);

    commandList->SetComputeRootSignature(computeRootSignature_);
    commandList->SetPipelineState(computePSO_Emit_); 
    commandList->SetComputeRootConstantBufferView(0, emitterBuffer_->GetGPUVirtualAddress());

    UINT groupCountX = (emitCount + 63) / 64;
    commandList->Dispatch(groupCountX, 1, 1);
}

void GPUParticleSystem::Update(ID3D12GraphicsCommandList* commandList, float deltaTime)
{
    totalTime_ += deltaTime;

    EmitterData cbData = {};
    cbData.deltaTime = deltaTime;

    void* mappedPtr = nullptr;
    emitterBuffer_->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, &cbData, sizeof(EmitterData));
    emitterBuffer_->Unmap(0, nullptr);

    commandList->SetComputeRootSignature(computeRootSignature_);
    commandList->SetPipelineState(computePSO_Update_); 
    commandList->SetComputeRootConstantBufferView(0, emitterBuffer_->GetGPUVirtualAddress());

    UINT groupCountX = (kMaxParticles + 63) / 64;
    commandList->Dispatch(groupCountX, 1, 1);
}