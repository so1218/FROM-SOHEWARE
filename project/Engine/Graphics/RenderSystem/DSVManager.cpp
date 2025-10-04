#include "DSVManager.h"

Microsoft::WRL::ComPtr <ID3D12Resource> DSVManager::CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height)
{
    // 生成するResourceの生成
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = width;// Textureの幅
    resourceDesc.Height = height;// Textureの高さ
    resourceDesc.MipLevels = 1;// mipmapの数
    resourceDesc.DepthOrArraySize = 1;// 奥行き or 配列Textureの配列数
    resourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;// DepthStencilとして利用可能なフォーマット
    resourceDesc.SampleDesc.Count = 1;// サンプリングカウント。1固定。
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;// 2次元
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;// DepthStencilとして使う通知

    // 利用するHeapの設定
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    // 深度値のクリア設定
    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.DepthStencil.Depth = 1.0f;// 1.0f(最大値)でクリア
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;// フォーマット。Resourceと合わせる

    // Resourceの生成
    Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,// Heapの設定
        D3D12_HEAP_FLAG_NONE,// Heapの特殊な設定。特になし
        &resourceDesc,// Resourceの設定
        D3D12_RESOURCE_STATE_DEPTH_WRITE,// 深度値を書き込む状態にしておく
        &depthClearValue,// Clear最適値,
        IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));

    return resource;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DSVManager::CreateDSVHeapAndView(ID3D12Device* device,
    ID3D12Resource* depthResource) {

    // DSV Heap の作成
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsvHeap));
    if (FAILED(hr)) {
        OutputDebugStringA("DescriptorHeap 作成に失敗しました\n");
        return nullptr;
    }
    
    assert(SUCCEEDED(hr));

    // DSV の作成
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(depthResource, &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

    return dsvHeap;
}