#include "pch.h"
#include "Camera.h"

void Camera::Initialize()
{
    // 初期化処理
    worldTransform_.Initialize();

    // カメラの初期位置を設定
    worldTransform_.translation_ = { 0.0f, 5.0f, -50.0f };

    // カメラパラメータの初期化
    fovY_ = 0.45f;
    aspectRatio_ = 16.0f / 9.0f;
    nearClip_ = 0.1f;
    farClip_ = 500.0f;

    // 行列を初期化しておく
    UpdateProjectionMatrix(); 
    UpdateViewProjectionMatrix();
}
