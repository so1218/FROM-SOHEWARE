#include "CameraManager.h"
#include "BufferManager.h" 
#include "Camera.h"        
#include <cassert>         

void CameraManager::Initialize(ID3D12Device * device)
{
    // Camera定数バッファを作成
    cameraResource_ = BufferManager::CreateBufferResource(device, sizeof(FrameData));

    // マッピング
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&frameData_));

    // 初期値の設定
    frameData_->cameraWorldPosition = { 0.0f, 0.0f, -10.0f }; // カメラ位置
    frameData_->nearClip = 0.1f;
    frameData_->farClip = 1000.0f;
}

void CameraManager::Update(Camera* camera)
{
    if (!camera) return;

    frameData_->cameraWorldPosition = camera->GetTranslation();

    frameData_->nearClip = camera->GetNearClip();
    frameData_->farClip = camera->GetFarClip();
}