#include "RTVManager.h"
#include "DescriptorHeapManager.h"
#include "Engine.h"

void RTVManager::Initialize(ID3D12Device* device, IDXGISwapChain4* swapChain, uint32_t bufferCount, DescriptorHeapManager* descriptorManager)
{
    backBufferCount = bufferCount;

    // 配列サイズを調整
    rtvHandles.resize(bufferCount);
    swapChainResources.resize(bufferCount);

    // RTVヒープを作成（Shader内で使用しないのでShaderVisibleはfalse）
    rtvDescriptorHeap_ = descriptorManager->CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, bufferCount, false);

    // ヒープ先頭のCPUハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStartCPUHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    // RTVディスクリプタのサイズを取得
    descriptorSizeRTV_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // RTVの基本設定
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // SwapChainのバックバッファから各RTVを作成
    for (uint32_t i = 0; i < backBufferCount; ++i)
    {
        HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
        assert(SUCCEEDED(hr));

        // CPUハンドルの位置を計算
        rtvHandles[i].ptr = rtvHeapStartCPUHandle.ptr + (SIZE_T)i * descriptorSizeRTV_;

        // RTVを作成
        device->CreateRenderTargetView(swapChainResources[i].Get(), &rtvDesc, rtvHandles[i]);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE RTVManager::GetCurrentBackBufferRTVCPUHandle(SwapChain* swapChainManager)
{
    UINT backBufferIndex = swapChainManager->GetSwapChain()->GetCurrentBackBufferIndex();
    assert(backBufferIndex < rtvHandles.size());
    return rtvHandles[backBufferIndex];
}

void OffscreenRTVManager::Initialize(ID3D12Device* device, SRVManager* srvManager, DescriptorHeapManager* descriptorManager, UINT rtvDescriptorCount)
{
    device_ = device;
    srvManager_ = srvManager;
    rtvDescriptorCount_ = rtvDescriptorCount;

    // RTVヒープを作成
    rtvDescriptorHeap_ = descriptorManager->CreateDescriptorHeap(device_, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtvDescriptorCount_, false);
    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    rtvHeapStart_ = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    offscreenRTVHandles_.resize(rtvDescriptorCount_);

    createdRTVCount_ = 0;
    clearColor_ = Vector4(0.03f, 0.03f, 0.03f, 1.0f);
}

std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, D3D12_CPU_DESCRIPTOR_HANDLE>
OffscreenRTVManager::CreateOffscreenRenderTarget(UINT width, UINT height, Vector4 clearColor)
{
    UINT rtvIndex = createdRTVCount_;
    assert(rtvIndex < rtvDescriptorCount_);
    createdRTVCount_++;

    // テクスチャのリソース作成
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    clearValue.Color[0] = clearColor.x;
    clearValue.Color[1] = clearColor.y;
    clearValue.Color[2] = clearColor.z;
    clearValue.Color[3] = clearColor.w;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    Microsoft::WRL::ComPtr<ID3D12Resource> texture;
    HRESULT hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(&texture)
    );
    assert(SUCCEEDED(hr));

    offscreenTextures_.push_back(texture);

    // RTV作成
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeapStart_;
    rtvHandle.ptr += rtvIndex * rtvDescriptorSize_;
    device_->CreateRenderTargetView(texture.Get(), nullptr, rtvHandle);
    offscreenRTVHandles_[rtvIndex] = rtvHandle;

    D3D12_SHADER_RESOURCE_VIEW_DESC sceneSrvDesc = {};
    sceneSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    sceneSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    sceneSrvDesc.Texture2D.MipLevels = 1;
    sceneSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // SRVを作成し、offscreenSrvIndex_に保存する
    offscreenSrvIndex_ = srvManager_->CreateSRV(texture.Get(), sceneSrvDesc);

    // 作成したリソースとRTVハンドルを返す
    return { texture, rtvHandle };
}
