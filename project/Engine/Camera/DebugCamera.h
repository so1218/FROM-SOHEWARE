#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Camera.h"

/// <summary>
/// デバッグカメラ
/// </summary>
class DebugCamera
{
public:
    DebugCamera()
    {
        // 初期化処理
        viewMatrix_ = Matrix4x4::MakeIdentity();
        worldTransform_.scale_ = { 1, 1, 1 };
        worldTransform_.rotation_ = { 0, 0, 0 };
        worldTransform_.translation_ = { 0, 0, -100 };
    }

    void Initialize();
    void Update();

    // カメラ取得(読み取り専用）
    const Camera& GetCamera() const { return camera_; }

    // ビュー射影行列取得
    Matrix4x4 GetViewMatrix();
    Matrix4x4 GetProjectionMatrix(){ return Matrix4x4::MakePerspectiveFov(fovY_, aspectRatio_, nearClip_, farClip_); }
    Matrix4x4 GetViewProjectionMatrix(){ return GetViewMatrix() * GetProjectionMatrix(); }

    // カメラパラメータ設定
    void SetFovY(float fovY) { fovY_ = fovY; UpdateProjectionMatrix(); }
    void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio; UpdateProjectionMatrix(); }

    void SetNearClip(float nearClip) { nearClip_ = nearClip; UpdateProjectionMatrix(); }
    void SetFarClip(float farClip) { farClip_ = farClip; UpdateProjectionMatrix(); }

    // target
    Vector3 GetTarget() const { return target_; }
    void SetTarget(const Vector3& target) { target_ = target; }

    // distance
    float GetDistance() const { return distance_; }
    void SetDistance(float distance) { distance_ = distance; }

    // currentPitch
    float GetCurrentPitch() const { return currentPitch_; }
    void SetCurrentPitch(float pitch) { currentPitch_ = pitch; }

    // currentYaw
    float GetCurrentYaw() const { return currentYaw_; }
    void SetCurrentYaw(float yaw) { currentYaw_ = yaw; }

    // 各速度設定
    float GetDragSpeed() { return dragSpeed_; }
    float GetRotateSpeed() { return rotateSpeed_; }
    float GetZoomSpeed() { return zoomSpeed_; }

    void SetEnabled(bool enabled) { isEnabled_ = enabled;}
    bool IsEnabled() const { return isEnabled_; }

    void SetDragSpeed(float speed) { dragSpeed_ = speed; }
    void SetRotateSpeed(float speed) { rotateSpeed_ = speed; }
    void SetZoomSpeed(float speed) { zoomSpeed_ = speed; }

private:
    void UpdateProjectionMatrix()
    {
        projectionMatrix_ = Matrix4x4::MakePerspectiveFov(fovY_, aspectRatio_, nearClip_, farClip_);
    }

private:
    // 内部カメラ
    Camera camera_;

    // 行列
    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_ = Matrix4x4::MakePerspectiveFov(fovY_, aspectRatio_, nearClip_, farClip_);
    Matrix4x4 cameraMatrix_;

    // ワールド変換
    WorldTransform worldTransform_;

    // カメラ制御パラメータ
    Vector3 target_ = { 0, 0, 0 }; // 注視点
    float distance_ = 0.0f;
    Vector3 upVector_ = { 0, 1, 0 };
    Vector3 cameraWorldPosition_ = { 0, 0, 0 };

    float currentPitch_ = 0.0f;
    float currentYaw_ = 0.0f;

    // パラメータ(マウス操作など用）
    float fovY_ = 0.45f;
    float aspectRatio_ = 1280.0f / 720.0f;
    float nearClip_ = 0.1f;
    float farClip_ = 1000.0f;

    float dragSpeed_ = 1.0f;
    float rotateSpeed_ = 1.0f;
    float zoomSpeed_ = 1.0f;

	float minDistance_ = 1.0f; 

    // デバッグカメラの有効/無効フラグ
    bool isEnabled_;
};