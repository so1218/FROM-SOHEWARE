#include "RTVManager.h"
#include "DescriptorHeapManager.h"
#include "Engine.h"

void RTVManager::Initialize(ID3D12Device* device, IDXGISwapChain4* swapChain, uint32_t bufferCount, uint32_t descriptorSizeRTV, DescriptorHeapManager* descriptorManager)
{
    backBufferCount = bufferCount;

    // 配列サイズを調整
    rtvHandles.resize(bufferCount);
    swapChainResources.resize(bufferCount);

    // RTV用のヒープを作成。RTVはShader内で触るものではないので、ShaderVisibleはfalse
    rtvDescriptorHeap_ = descriptorManager->CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, bufferCount, false);

    // RTVディスクリプタヒープの先頭CPUハンドルを取得
    // rtvDescriptorHeap_はすでにDescriptorManager::CreateDescriptorHeapで生成されているため、
    // そのID3D12DescriptorHeapインターフェースから直接ハンドルを取得するのが最も確実です。
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStartCPUHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    // RTVディスクリプタのサイズを取得 (念のため、descriptorSizeRTVではなくデバイスから取得します)
    UINT rtvDescriptorIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // RTVの設定
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2dテクスチャとして読み込む

    // SwapChainからResourceを引っ張ってきて、各RTVを作成
    for (uint32_t i = 0; i < backBufferCount; ++i)
    {
        // SwapChainのバッファを取得
        HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
        assert(SUCCEEDED(hr));

        // rtvHandles[i] に設定するCPUハンドルの位置を計算
        // ヒープの先頭ハンドルから、i * IncrementSize 分オフセットする
        rtvHandles[i].ptr = rtvHeapStartCPUHandle.ptr + (SIZE_T)i * rtvDescriptorIncrementSize;

        // RTVを作成し、計算したハンドル位置に書き込む
        device->CreateRenderTargetView(
            swapChainResources[i].Get(),
            &rtvDesc,
            rtvHandles[i]
        );
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE RTVManager::GetCurrentBackBufferRTVCPUHandle(SwapChain* swapChainManager)
{
    UINT backBufferIndex = swapChainManager->GetSwapChain()->GetCurrentBackBufferIndex();
    assert(backBufferIndex < rtvHandles.size());
    return rtvHandles[backBufferIndex];
}

void OffscreenRTVManager::Initialize(ID3D12Device* device, DescriptorHeapManager* descriptorManager, UINT rtvDescriptorCount)
{
    device_ = device;
    rtvDescriptorCount_ = rtvDescriptorCount;
    rtvDescriptorHeap_ = descriptorManager->CreateDescriptorHeap(device_, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtvDescriptorCount_, false);
    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // ヒープの先頭ハンドルを取得
    rtvHeapStart_ = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    // RTVハンドル配列を初期化
    offscreenRTVHandles_.resize(rtvDescriptorCount_);

    // SRV用デスクリプタヒープ作成
    {
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = rtvDescriptorCount_; // オフスクリーンテクスチャの数に応じて調整
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダーで使用するために必須
        device_->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvDescriptorHeap_));
    }

    // Shader-Invisible ヒープ（CPU専用）作成
    {
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = rtvDescriptorCount_;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // Shader-Invisible
        device_->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&cpuOnlySRVDescriptorHeap_));
    }

    srvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    createdRTVCount_ = 0;

    clearColor_ = Vector4(0.03f, 0.03f, 0.03f, 1.0f);
}

uint32_t OffscreenRTVManager::CreateOffscreenRenderTarget(UINT width, UINT height, Vector4 clearColor)
{
    UINT rtvIndex = createdRTVCount_;
    assert(rtvIndex < rtvDescriptorCount_);
    createdRTVCount_++;

    // リソース作成
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

    // RTV 作成
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeapStart_;
    rtvHandle.ptr += rtvIndex * rtvDescriptorSize_;
    device_->CreateRenderTargetView(texture.Get(), nullptr, rtvHandle);
    offscreenRTVHandles_[rtvIndex] = rtvHandle;

    // SRV 作成（両方のヒープに）
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // Shader-Visible（GPU用）
    D3D12_CPU_DESCRIPTOR_HANDLE visibleCPUHandle = srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    visibleCPUHandle.ptr += rtvIndex * srvDescriptorSize_;
    device_->CreateShaderResourceView(texture.Get(), &srvDesc, visibleCPUHandle);

    // Shader-Invisible（CPU Copy用）
    D3D12_CPU_DESCRIPTOR_HANDLE invisibleCPUHandle = cpuOnlySRVDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    invisibleCPUHandle.ptr += rtvIndex * srvDescriptorSize_;
    device_->CreateShaderResourceView(texture.Get(), &srvDesc, invisibleCPUHandle);

    // GPUハンドル（描画時に使う）
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += rtvIndex * srvDescriptorSize_;

    // SRV 登録（Copy元は Shader-Invisible）
    AllocateAndRegisterSRV(invisibleCPUHandle, gpuHandle);

    return rtvIndex;
}

uint32_t OffscreenRTVManager::CreateDepthTexture(UINT width, UINT height)
{
    UINT rtvIndex = createdRTVCount_;
    assert(rtvIndex < rtvDescriptorCount_);
    createdRTVCount_++;

    // 深度テクスチャ用のリソース記述
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    Microsoft::WRL::ComPtr<ID3D12Resource> texture;
    HRESULT hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&texture)
    );
    assert(SUCCEEDED(hr));

    offscreenTextures_.push_back(texture);

    // DSVハンドル取得
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    dsvHandle.ptr += rtvIndex * dsvDescriptorSize_;

    // DSVビュー記述子を作成
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    // DSVビュー作成
    device_->CreateDepthStencilView(texture.Get(), &dsvDesc, dsvHandle);

    // SRV作成（深度テクスチャ用）
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // Shader-Visible（GPU用）
    D3D12_CPU_DESCRIPTOR_HANDLE visibleCPUHandle = srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    visibleCPUHandle.ptr += rtvIndex * srvDescriptorSize_;
    device_->CreateShaderResourceView(texture.Get(), &srvDesc, visibleCPUHandle);

    // Shader-Invisible（CPU Copy用）
    D3D12_CPU_DESCRIPTOR_HANDLE invisibleCPUHandle = cpuOnlySRVDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    invisibleCPUHandle.ptr += rtvIndex * srvDescriptorSize_;
    device_->CreateShaderResourceView(texture.Get(), &srvDesc, invisibleCPUHandle);

    // GPUハンドル（描画時に使う）
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += rtvIndex * srvDescriptorSize_;

    AllocateAndRegisterSRV(invisibleCPUHandle, gpuHandle);

    return rtvIndex;
}

uint32_t OffscreenRTVManager::AllocateAndRegisterSRV(
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    SRVEntry entry{ cpuHandle, gpuHandle };
    srvEntries_.push_back(entry);
    return static_cast<uint32_t>(srvEntries_.size() - 1);
}