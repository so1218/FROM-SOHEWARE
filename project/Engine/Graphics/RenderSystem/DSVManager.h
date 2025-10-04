#pragma once

#include <d3d12.h>             
#include <dxgi1_6.h>           
#include <d3dcommon.h>               
#include <assert.h>      
#include <cstdint>
#include <wrl.h>

class DSVManager
{
public:
    // DepthStencilTexture関数
    static Microsoft::WRL::ComPtr <ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

    // DSVヒープとViewを作成（オフスクリーン用など）
    static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDSVHeapAndView(ID3D12Device* device,
        ID3D12Resource* depthResource);
};