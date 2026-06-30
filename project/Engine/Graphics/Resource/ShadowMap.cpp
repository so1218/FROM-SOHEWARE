#include "pch.h"
#include "ShadowMap.h"
#include "SRVManager.h" 

namespace FE
{

void ShadowMap::Initialize(ID3D12Device* device, int width, int height, SRVManager* srvManager)
{
    srvManager_ = srvManager;

    width_ = static_cast<UINT>(width);
    height_ = static_cast<UINT>(height);

    // ビューポートとシザー矩形を事前計算
    viewport_ = { 0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_), 0.0f, 1.0f };
    scissorRect_ = { 0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_) };

    // リソース設定 
    D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32_TYPELESS,
        width, height,
        kNumCascades,
        1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL // 深度として使うフラグ
    );

    // クリア値の設定
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT; // 深度フォーマット
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    // リソース生成
    HRESULT hr = device->CreateCommittedResource(
        &heapProps, 
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(&shadowResource_)
    );
    assert(SUCCEEDED(hr));

    // DSVの作成
    // 専用のDSVヒープを作る
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = kNumCascades;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap_));
    assert(SUCCEEDED(hr));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY; 
    dsvDesc.Texture2DArray.ArraySize = 1; // 1枚ずつ書き込む

    auto dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    uint32_t descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    for (uint32_t i = 0; i < kNumCascades; ++i)
    {
        dsvDesc.Texture2DArray.FirstArraySlice = i; // 書き込むレイヤーを指定
        device->CreateDepthStencilView(shadowResource_.Get(), &dsvDesc, dsvHandle);
        dsvHandle.ptr += descriptorSize; // 次のハンドルへ進める
    }

    // SRVの作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = kNumCascades;          
    srvDesc.Texture2DArray.PlaneSlice = 0;

    srvIndex_ = srvManager_->CreateSRV(shadowResource_.Get(), srvDesc);

    dsvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

D3D12_GPU_DESCRIPTOR_HANDLE ShadowMap::GetSRVHandle() const
{
    // 保存したインデックスを使ってGPUハンドルを返す
    return srvManager_->GetSRVHandleGPU(srvIndex_);
}

// コピー用のCPUハンドルを返す
D3D12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetSRVHandleCPU() const
{
    return srvManager_->GetSRVHandleCPU_ForCopying(srvIndex_);
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetDSVHandle(uint32_t cascadeIndex) const
{
    auto handle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += dsvDescriptorSize_ * cascadeIndex;

    return handle;
}

void ShadowMap::TransitionToDepthWrite(ID3D12GraphicsCommandList* commandList)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        shadowResource_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );
    commandList->ResourceBarrier(1, &barrier);
}

void ShadowMap::TransitionToRead(ID3D12GraphicsCommandList* commandList)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        shadowResource_.Get(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    commandList->ResourceBarrier(1, &barrier);
}

void ShadowMap::BeginPass(ID3D12GraphicsCommandList* cmdList, uint32_t cascadeIndex)
{
    // レンダーターゲット(DSV)のセット
    D3D12_CPU_DESCRIPTOR_HANDLE shadowDSV = GetDSVHandle(cascadeIndex);
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &shadowDSV);

    // クリア
    cmdList->ClearDepthStencilView(shadowDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // ビューポートとシザーをセット
    cmdList->RSSetViewports(1, &viewport_);
    cmdList->RSSetScissorRects(1, &scissorRect_);
}

}