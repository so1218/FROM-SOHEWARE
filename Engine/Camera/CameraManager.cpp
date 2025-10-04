#include "CameraManager.h"
#include "BufferManager.h" 
#include "Camera.h"        
#include <cassert>         

// Initialize 関数
// カメラ用の定数バッファリソースを作成し、CPUからマップする
void CameraManager::Initialize(ID3D12Device * device)
{
    // Camera 定数バッファを作成
    cameraResource_ = BufferManager::CreateBufferResource(device, sizeof(Camera));

    // マッピング（CPU側で編集できるように）
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));

    // 初期値の設定
    cameraData_->worldPosition = { 0.0f, 0.0f, -10.0f }; // カメラ位置
    cameraData_->padding0 = 0.0f;
}