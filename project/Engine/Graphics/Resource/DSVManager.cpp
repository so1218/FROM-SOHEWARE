#include "DSVManager.h"
#include "DescriptorHeapManager.h"
#include "Logger.h"
#include "SRVManager.h"

void DSVManager::Initialize(ID3D12Device* device, DescriptorHeapManager* descriptorManager, SRVManager* srvManager, UINT dsvCount)
{
    device_ = device;
    srvManager_ = srvManager;
    maxDSVCount_ = dsvCount;
    createdDSVCount_ = 0;

    // DSV用ヒープを作成（Shader-Visibleではない）
    dsvHeap_ = descriptorManager->CreateDescriptorHeap(
        device_,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        dsvCount,
        false
    );

    // DSVディスクリプタのサイズとヒープ先頭ハンドルを取得
    dsvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    dsvHeapStart_ = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
}

// リソース作成とDSV作成を同時に行う
D3D12_CPU_DESCRIPTOR_HANDLE DSVManager::CreateDepthStencilView(
    UINT width,
    UINT height,
    Microsoft::WRL::ComPtr<ID3D12Resource>& outResource)
{
    // ヒープの空きスロット確認
    assert(createdDSVCount_ < maxDSVCount_);

    // 深度ステンシル用リソースを作成
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.MipLevels = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // ★ここでSRVも作成して登録する
    // フォーマット注意: R24G8_TYPELESS の場合、
    // Depthを読むには DXGI_FORMAT_R24_UNORM_X8_TYPELESS を使う
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    // SRVManagerを使ってSRV作成
    uint32_t srvIndex = srvManager_->CreateSRV(outResource.Get(), srvDesc);
    depthSrvIndices_.push_back(srvIndex); // インデックスを保存

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.DepthStencil.Depth = 1.0f;
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    HRESULT hr = device_->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&outResource)
    );
    assert(SUCCEEDED(hr));

    // 作成したリソースを保持
    depthTextures_.push_back(outResource);

    // 空きスロットのハンドルを計算
    UINT dsvIndex = createdDSVCount_++;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeapStart_;
    dsvHandle.ptr += (SIZE_T)dsvIndex * dsvDescriptorSize_;

    // DSVの設定
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    // DSVをヒープに作成
    device_->CreateDepthStencilView(outResource.Get(), &dsvDesc, dsvHandle);

    LOG_INFO("DSV Created at Index: {}", dsvIndex);

    // 作成したDSVのCPUハンドルを返す
    return dsvHandle;
}

