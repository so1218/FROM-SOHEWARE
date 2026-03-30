#include "pch.h"
#include "BufferManager.h"

namespace FE
{

Microsoft::WRL::ComPtr<ID3D12Resource> BufferManager::CreateBufferResource(
    ID3D12Device* device,
    size_t sizeInBytes,
    D3D12_RESOURCE_FLAGS flags, 
    D3D12_RESOURCE_STATES initialState, 
    D3D12_HEAP_TYPE heapType
)
{
    // ヒーププロパティの設定
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = heapType; 

    // バッファリソースの設定
    D3D12_RESOURCE_DESC resourceDesc = {}; 
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // バッファリソース
    resourceDesc.Width = sizeInBytes; // サイズを指定
    resourceDesc.Height = 1; // バッファの場合は高さは1
    resourceDesc.DepthOrArraySize = 1; // バッファの場合は1
    resourceDesc.MipLevels = 1; // バッファにはミップマップレベルは不要
    resourceDesc.SampleDesc.Count = 1; // サンプル数は1
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // バッファのレイアウト
    resourceDesc.Flags = flags; 

    // 実際にバッファリソースを作成
    Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties, // ヒーププロパティ
        D3D12_HEAP_FLAG_NONE, // ヒープフラグ
        &resourceDesc, // リソースの説明
        initialState,
        nullptr,
        IID_PPV_ARGS(&bufferResource) // リソースのポインタを受け取る
    );

    assert(SUCCEEDED(hr));

    // 成功したかどうかを確認
    if (FAILED(hr))
    {
        // エラーハンドリング
        return nullptr;
    }

    return bufferResource;
}

}