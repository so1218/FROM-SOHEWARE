#pragma once

#include <d3d12.h>  
#include <dxgi1_4.h>    
#include <fstream>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "ImGuizmo.h"
#include "Vector2.h"

class WorldTransform;
class Camera;

class ImGuiManager
{
public:
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
    static void SaveFile(const std::string& filename);
    static void OpenFile(const std::string& filename);
	static void EndFrame(ID3D12GraphicsCommandList* commandList);
	// ImGuiの終了処理
	static void Finalize();
    // リセット要求を確認・取得する
    static bool GetSceneResetRequested() { return resetSceneSize_; }
    static void ClearSceneResetRequested() { resetSceneSize_ = false; }

    // Gizmoを出せるようにする関数
    static void DrawGizmo(WorldTransform& transform, const Camera& camera);

    // シーンビューの情報をセットする
    static void SetSceneViewRect(const Vector2& min, const Vector2& size, bool isHovered);

    // 外部から情報を取る用
    static bool IsSceneHovered() { return isSceneHovered_; }

    static Vector2 GetSceneViewportMin() { return sceneRectMin_; }
    static Vector2 GetSceneViewportSize() { return sceneRectSize_; }

    static bool dockInitialized_;
    static bool resetSceneSize_;

private:
    // 操作モードを保持する変数
    static int gizmoOperation_;

    // 状態保持用
    static Vector2 sceneRectMin_;
    static Vector2 sceneRectSize_;
    static bool isSceneHovered_;
};

