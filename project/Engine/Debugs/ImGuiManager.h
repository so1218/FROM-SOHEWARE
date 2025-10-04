#pragma once

#include <d3d12.h>  
#include <dxgi1_4.h>    
#include <fstream>
#include <iostream>

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

class ImGuiManager
{
public:
	// ImGuiの初期化
	static void Initialize(
        HWND hwnd,
        ID3D12Device* device,
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc,
        const DXGI_SWAP_CHAIN_DESC1& swapChainDesc,
        ID3D12DescriptorHeap* srvDescriptorHeap,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

	static void BeginFrame();
    static void DrawMenuBar();
    static void SaveFile();
    static void OpenFile(const std::string& filename);
	static void EndFrame(ID3D12GraphicsCommandList* commandList);
	// ImGuiの終了処理
	static void Finalize();

};

