#include "pch.h"
#include "GrassFirePass.h"
#include "Engine.h"
#include "GraphicsDevice.h"
#include "RootSignatureManager.h"
#include "PSOManager.h"
#include "SRVManager.h"
#include "EnvironmentManager.h"
#include "TimeManager.h"
#include "PIXColors.h"

namespace FS
{

namespace
{

constexpr DXGI_FORMAT kFireMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
constexpr DXGI_FORMAT kReadbackFormat = DXGI_FORMAT_R32_FLOAT;

// パス専用ヒープの置き場所
enum HeapSlot : uint32_t
{
    kSlotPreviousFireMap = 0,
    kSlotDensityMap,
    kSlotFireMapUav,
    kSlotReadbackSource,
    kSlotReadbackUav,
    kSlotCount,
};

}

GrassFirePass::~GrassFirePass()
{
    SRVManager* srvManager = engine_ ? engine_->GetSRVManager() : nullptr;
    if (!srvManager) return;

    auto Free = [srvManager](uint32_t index) { if (index != 0) srvManager->FreeSRV(index); };
    for (int index = 0; index < 2; ++index)
    {
        Free(srvIndices_[index]);
        Free(uavIndices_[index]);
    }
    Free(readbackSrvIndex_);
    Free(readbackUavIndex_);
}

void GrassFirePass::CreateTexture(ID3D12Device* device, DXGI_FORMAT format, uint32_t size, const wchar_t* name,
    Microsoft::WRL::ComPtr<ID3D12Resource>& resource, uint32_t& srvIndex, uint32_t& uavIndex)
{
    SRVManager* srvManager = engine_->GetSRVManager();

    const CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format, size, size, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&resource));
    resource->SetName(name);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavIndex = srvManager->Allocate();
    device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(uavIndex));
    device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU(uavIndex));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvIndex = srvManager->Allocate();
    device->CreateShaderResourceView(resource.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(srvIndex));
    device->CreateShaderResourceView(resource.Get(), &srvDesc, srvManager->GetSRVHandleCPU(srvIndex));
}

void GrassFirePass::Initialize(Engine* engine, PSOManager* psoManager)
{
    engine_ = engine;
    psoManager_ = psoManager;

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();

    constantBuffer_ = BufferManager::CreateMappedConstantBuffer<GrassFireConstants>(device, &mappedConstants_);
    *mappedConstants_ = {};

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = kSlotCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));
    passHeap_->SetName(L"GrassFire_Heap");

    CreateTexture(device, kFireMapFormat, kMapResolution, L"GrassFireMap_0", fireMaps_[0], srvIndices_[0], uavIndices_[0]);
    CreateTexture(device, kFireMapFormat, kMapResolution, L"GrassFireMap_1", fireMaps_[1], srvIndices_[1], uavIndices_[1]);
    CreateTexture(device, kReadbackFormat, kReadbackResolution, L"GrassFireReadbackMap", readbackTexture_, readbackSrvIndex_, readbackUavIndex_);

    // 行の幅は 256 バイトにそろえる決まりがあるので、置き方は D3D12 に決めてもらう
    const D3D12_RESOURCE_DESC readbackDesc = readbackTexture_->GetDesc();
    uint64_t readbackSize = 0;
    device->GetCopyableFootprints(&readbackDesc, 0, 1, 0, &readbackFootprint_, nullptr, nullptr, &readbackSize);

    const CD3DX12_HEAP_PROPERTIES readbackHeapProperties(D3D12_HEAP_TYPE_READBACK);
    const CD3DX12_RESOURCE_DESC readbackBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(readbackSize);
    device->CreateCommittedResource(&readbackHeapProperties, D3D12_HEAP_FLAG_NONE, &readbackBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readbackBuffer_));
    readbackBuffer_->SetName(L"GrassFireReadbackBuffer");
}

void GrassFirePass::ReadBack()
{
    // GPU が描き終わるのを毎フレーム待っているので、前のフレームのコピーはもう終わっている
    if (!hasPendingReadback_) return;
    hasPendingReadback_ = false;

    uint8_t* mapped = nullptr;
    if (FAILED(readbackBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mapped)))) return;

    readbackValues_.resize(static_cast<size_t>(kReadbackResolution) * kReadbackResolution);
    for (uint32_t row = 0; row < kReadbackResolution; ++row)
    {
        const float* source = reinterpret_cast<const float*>(mapped + readbackFootprint_.Offset + readbackFootprint_.Footprint.RowPitch * row);
        std::memcpy(&readbackValues_[static_cast<size_t>(row) * kReadbackResolution], source, sizeof(float) * kReadbackResolution);
    }

    const D3D12_RANGE noWrite = { 0, 0 };
    readbackBuffer_->Unmap(0, &noWrite);
}

void GrassFirePass::Execute(ID3D12GraphicsCommandList* cmdList, const GrassFireConstants& settings,
    const std::vector<GrassFireIgnition>& ignitions, uint32_t densityMapSrvIndex, bool isResetRequested)
{
    ReadBack();

    // 作った直後の地図は中身が決まっていないので、最初に必ず 0 で埋める
    const bool isReset = isResetRequested || needsClear_;
    const bool hasArea = densityMapSrvIndex != 0 && settings.areaSize.x > 0.0f && settings.areaSize.y > 0.0f;
    if (isReset)
    {
        readbackValues_.clear();
    }
    if (!hasArea && !isReset)
    {
        mappedConstants_->isActive = 0;
        return;
    }

    const uint32_t readIndex = latestIndex_;
    const uint32_t writeIndex = 1 - latestIndex_;

    PIXScopedEvent(cmdList, PIXColors::Compute, "Grass Fire Pass");

    // GPU が描き終わるのを毎フレーム待っているので、ここで直接書き換える
    GrassFireConstants& constants = *mappedConstants_;
    constants = settings;
    constants.deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    constants.resetRequested = isReset ? 1 : 0;
    constants.isActive = hasArea ? 1 : 0;
    constants.ignitionCount = static_cast<uint32_t>(std::min<size_t>(ignitions.size(), MAX_GRASS_FIRE_IGNITIONS));
    for (uint32_t index = 0; index < constants.ignitionCount; ++index)
    {
        constants.ignitions[index] = ignitions[index];
    }

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    SRVManager* srvManager = engine_->GetSRVManager();
    const uint32_t handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const D3D12_CPU_DESCRIPTOR_HANDLE heapStartCPU = passHeap_->GetCPUDescriptorHandleForHeapStart();
    const D3D12_GPU_DESCRIPTOR_HANDLE heapStartGPU = passHeap_->GetGPUDescriptorHandleForHeapStart();
    auto CopyToHeap = [&](uint32_t slot, uint32_t sourceIndex)
        {
            device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(heapStartCPU, slot, handleSize),
                srvManager->GetSRVHandleCPU_ForCopying(sourceIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        };
    auto HeapGPU = [&](uint32_t slot) { return CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStartGPU, slot, handleSize); };
    auto Transition = [&](ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
        {
            const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
            cmdList->ResourceBarrier(1, &barrier);
        };

    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 燃え具合を進める
    CopyToHeap(kSlotPreviousFireMap, srvIndices_[readIndex]);
    CopyToHeap(kSlotDensityMap, hasArea ? densityMapSrvIndex : srvIndices_[readIndex]);
    CopyToHeap(kSlotFireMapUav, uavIndices_[writeIndex]);

    Transition(fireMaps_[writeIndex].Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->SetComputeRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("GrassFireCS"));
    cmdList->SetPipelineState(psoManager_->GetPSO("GrassFireCS"));
    cmdList->SetComputeRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(1, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(2, HeapGPU(kSlotPreviousFireMap));
    cmdList->SetComputeRootDescriptorTable(3, HeapGPU(kSlotFireMapUav));
    cmdList->Dispatch((kMapResolution + 7) / 8, (kMapResolution + 7) / 8, 1);
    Transition(fireMaps_[writeIndex].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

    latestIndex_ = writeIndex;
    needsClear_ = false;

    // CPU へ読み戻す小さな地図
    if (hasArea)
    {
        CopyToHeap(kSlotReadbackSource, srvIndices_[writeIndex]);
        CopyToHeap(kSlotReadbackUav, readbackUavIndex_);

        Transition(readbackTexture_.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->SetComputeRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("GrassFireReadbackCS"));
        cmdList->SetPipelineState(psoManager_->GetPSO("GrassFireReadbackCS"));
        cmdList->SetComputeRootDescriptorTable(0, HeapGPU(kSlotReadbackSource));
        cmdList->SetComputeRootDescriptorTable(1, HeapGPU(kSlotReadbackUav));
        cmdList->Dispatch((kReadbackResolution + 7) / 8, (kReadbackResolution + 7) / 8, 1);

        Transition(readbackTexture_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        const CD3DX12_TEXTURE_COPY_LOCATION destination(readbackBuffer_.Get(), readbackFootprint_);
        const CD3DX12_TEXTURE_COPY_LOCATION source(readbackTexture_.Get(), 0);
        cmdList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
        Transition(readbackTexture_.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        hasPendingReadback_ = true;
    }

    // 共通ヒープに戻す
    ID3D12DescriptorHeap* mainHeaps[] = { srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, mainHeaps);
}

}
