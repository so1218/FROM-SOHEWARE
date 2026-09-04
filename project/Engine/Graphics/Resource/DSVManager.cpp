#include "pch.h"
#include "DSVManager.h"
#include "DescriptorHeapManager.h"
#include "Logger.h"
#include "SRVManager.h"

namespace FE
{

void DSVManager::Initialize(
    ID3D12Device* device,
    DescriptorHeapManager* descriptorManager,
    SRVManager* srvManager,
    uint32_t dsvCount
)
{
    device_ = device;
    srvManager_ = srvManager;
    maxDSVCount_ = dsvCount;
    createdDSVCount_ = 0;

    // DSV用ディスクリプタヒープを作成（Shader Visible なし）
    dsvHeap_ = descriptorManager->CreateDescriptorHeap(
        device_,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        dsvCount,
        false
    );

    // DSVディスクリプタサイズとヒープ先頭ハンドルを取得
    dsvDescriptorSize_ =
        device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    dsvHeapStart_ = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
}

// 深度ステンシル用リソースとDSVを作成
D3D12_CPU_DESCRIPTOR_HANDLE DSVManager::CreateDepthStencilView(
    uint32_t width,
    uint32_t height,
    Microsoft::WRL::ComPtr<ID3D12Resource>& outResource
)
{
    // DSV作成数の上限チェック
    assert(createdDSVCount_ < maxDSVCount_);

    // 深度ステンシル用テクスチャの設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.MipLevels = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // デフォルトヒープを使用
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    // 深度初期化用のクリア値
    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.DepthStencil.Depth = 1.0f;
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // 深度ステンシルリソースを作成
    HRESULT hr = device_->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&outResource)
    );
    assert(SUCCEEDED(hr));

    // 深度テクスチャ参照用のSRVを作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    uint32_t srvIndex = srvManager_->CreateSRV(outResource.Get(), srvDesc);
    depthSrvIndices_.push_back(srvIndex);

    // リソースを管理リストに保持
    depthTextures_.push_back(outResource);

    // DSVの書き込み先ハンドルを計算
    uint32_t dsvIndex = createdDSVCount_++;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeapStart_;
    dsvHandle.ptr +=
        static_cast<SIZE_T>(dsvIndex) * dsvDescriptorSize_;

    // DSVの設定
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    // DSVをヒープに作成
    device_->CreateDepthStencilView(
        outResource.Get(),
        &dsvDesc,
        dsvHandle
    );

    LOG_INFO("DSV Created at Index: {}", dsvIndex);

    // 作成したDSVのCPUハンドルを返す
    return dsvHandle;
}

}