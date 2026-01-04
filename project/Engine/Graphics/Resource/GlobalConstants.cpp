#include "GlobalConstants.h"
#include "BufferManager.h" 
#include "Camera.h"        
#include "TimeManager.h"
#include "Engine.h"
#include <cassert>         

void GlobalConstants::Initialize(ID3D12Device* device)
{
    // 定数バッファを作成 (サイズは FrameData 構造体)
    constantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(FrameData));

    // マッピング
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&frameData_));

    // 初期化 (安全のためゼロクリア推奨)
    memset(frameData_, 0, sizeof(FrameData));

    // デフォルト値
    frameData_->nearClip = 0.1f;
    frameData_->farClip = 1000.0f;
    frameData_->viewProjectionMatrix = Matrix4x4::MakeIdentity(); // ※単位行列などで初期化
}

void GlobalConstants::Update(const Camera& camera)
{
    // カメラ情報の転送
    frameData_->cameraWorldPosition = camera.GetTranslation();
    frameData_->viewProjectionMatrix = camera.GetViewProjectionMatrix();

    Matrix4x4 view = camera.GetViewMatrix();
    frameData_->cameraRight = { view.m[0][0], view.m[1][0], view.m[2][0] };
    frameData_->cameraUp = { view.m[0][1], view.m[1][1], view.m[2][1] };

    frameData_->nearClip = camera.GetNearClip();
    frameData_->farClip = camera.GetFarClip();

    // ★ここが重要: カメラ以外のデータも更新する
    frameData_->gTime = TimeManager::GetInstance()->GetTotalTime();

    frameData_->iResolution = Vector2(1, 1);
    frameData_->screenResolution = Vector2(static_cast<float>(kClientWidth), static_cast<float>(kClientHeight));
}