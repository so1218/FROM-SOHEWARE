#include "pch.h"
#include "GPUParticleSystem.h"

void GPUParticleSystem::Initialize(ID3D12Device* device)
{
    // particleBuffer_, emitterBuffer_作成処理

    // C. FreeListバッファ (UAV) の作成
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

    // D. FreeListCounterバッファ (UAV) の作成
    uavDesc.Width = sizeof(uint32_t);
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &uavDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&freeListCounter_));

    // ここで初期化用ComputeShaderを走らせて、
    // freeListBuffer_[i] = i; と freeListCounter_[0] = kMaxParticles; を設定
}

// Emit (FreeListからインデックスを消費してパーティクル発生)
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
    commandList->SetPipelineState(computePSO_Emit_); // Emit用シェーダーをセット
    commandList->SetComputeRootConstantBufferView(0, emitterBuffer_->GetGPUVirtualAddress());

    // スレッドグループのディスパッチ (発生させる数に合わせてDispatch)
    UINT groupCountX = (emitCount + 63) / 64;
    commandList->Dispatch(groupCountX, 1, 1);
}

// Update (寿命計算とFreeListへの返却のみ行う)
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
    commandList->SetPipelineState(computePSO_Update_); // Update用シェーダーをセット
    commandList->SetComputeRootConstantBufferView(0, emitterBuffer_->GetGPUVirtualAddress());

    // 全パーティクルを検査して寿命更新＆回収 (10000 / 64)
    UINT groupCountX = (kMaxParticles + 63) / 64;
    commandList->Dispatch(groupCountX, 1, 1);
}