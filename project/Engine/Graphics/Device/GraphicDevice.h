#pragma once

#include <wrl.h>           
#include <dxgi1_6.h>              
#include <cassert>                
#include <windows.h> 
#include <fstream>
#include <d3d12.h>    

class GraphicDevice
{
public:
    void Initialize();
    void CreateFactory();
    void SelectAdapter();
    void CreateDevice();
    void EnableDebugLayer();

    // ゲッター
    ID3D12Device* GetDevice() const { return device_.Get(); }

private:
    // DXリソース
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
};
