#pragma once

#include <d3d12.h>                      
#include <cassert>                
#include <d3dcompiler.h> 
#include <wrl.h> 

class BufferManager
{
public:
    // 指定されたサイズのバッファリソースを作成
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(
        ID3D12Device* device,
        size_t sizeInBytes,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, // リソースフラグ
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ, // 初期状態
        D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD // ヒープタイプも柔軟に
    );
};