#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#include "DebugCamera.h"
#include "Input.h"

void DebugCamera::Initialize()
{
    // ワールド変換の初期化
    worldTransform_.Initialize();

    // カメラの初期設定
    target_ = { 0.0f, 0.0f, 0.0f }; // 注視点
    distance_ = 50.0f;             // 注視点からの距離
    upVector_ = { 0.0f, 1.0f, 0.0f }; // MakeLookAt用の上方向ベクトル

    // クォータニオンのための初期角度
    currentPitch_ = 0.4f;
    currentYaw_ = 0.0f;

    dragSpeed_ = 0.02f;
    rotateSpeed_ = 0.001f;
    zoomSpeed_ = 0.03f;

    isEnabled_ = true;

    // ワールド行列の更新
    worldTransform_.UpdateMatrix();
}

void DebugCamera::Update()
{
    if (!isEnabled_) return;

    // ズームはマウスホイールで操作
    if (Input::IsMouseButtonPressed(Input::MouseButton::Right)) {
        int wheelDelta = Input::GetMouseWheelDelta();
        distance_ -= wheelDelta * zoomSpeed_;

        Quaternion currentRotation = worldTransform_.rotationQuaternion_;
        Vector3 forward = currentRotation.RotateVector(Vector3(0.0f, 0.0f, 1.0f));
        if (distance_ < minDistance_) {
            // target を forward 方向に押す
            target_ += forward * (minDistance_ - distance_);
            distance_ = minDistance_;
        }
    }

    // マウス右ドラッグでカメラ回転（target中心の公転）
    if (Input::IsMouseButtonPressed(Input::MouseButton::Middle) && !Input::IsKeyPressed(DIK_LSHIFT))
    {
        int deltaX = Input::GetMouseState().lX;
        int deltaY = Input::GetMouseState().lY;

        currentYaw_ += deltaX * rotateSpeed_;
        currentPitch_ += deltaY * rotateSpeed_;
    }

    // コントローラーの右スティック入力によるカメラ回転
    if (Input::IsControllerConnected(0))
    {
        SHORT stickX = Input::GetRightStickX(0);
        SHORT stickY = Input::GetRightStickY(0);

        // スティックのデッドゾーン処理（誤操作防止）
        const int DEAD_ZONE = 8000;
        if (abs(stickX) > DEAD_ZONE || abs(stickY) > DEAD_ZONE)
        {
            currentYaw_ -= static_cast<float>(stickX) * 0.000001f;
            currentPitch_ += static_cast<float>(stickY) * 0.000001f;
        }
    }

    // マウス中ドラッグでターゲット
    if (Input::IsMouseButtonPressed(Input::MouseButton::Middle) && Input::IsKeyPressed(DIK_LSHIFT)) {
        Quaternion currentRotation = worldTransform_.rotationQuaternion_;
        Vector3 right = currentRotation.RotateVector(Vector3(1.0f, 0.0f, 0.0f));
        Vector3 up = currentRotation.RotateVector(Vector3(0.0f, 1.0f, 0.0f));

        int deltaX = Input::GetMouseState().lX;
        int deltaY = Input::GetMouseState().lY;

        target_ -= right * static_cast<float>(deltaX) * dragSpeed_;
        target_ += up * static_cast<float>(deltaY) * dragSpeed_;
    }

    // currentPitch_ と currentYaw_ を使って回転クォータニオンを作成し、常に反映
    Quaternion pitchQuaternion = Quaternion::FromAxisAngle({ 1.0f, 0.0f, 0.0f }, currentPitch_);
    Quaternion yawQuaternion = Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, currentYaw_);
    worldTransform_.SetRotation(yawQuaternion * pitchQuaternion);

    // カメラのワールド座標を計算
    Quaternion currentRotation = worldTransform_.rotationQuaternion_;
    Vector3 forward = currentRotation.RotateVector(Vector3(0.0f, 0.0f, 1.0f));
    cameraWorldPosition_ = target_ - forward * distance_;

    // ワールド行列を更新
    worldTransform_.UpdateMatrix();
}

Matrix4x4 DebugCamera::GetViewMatrix()
{
    // カメラのワールド行列を作る
    Matrix4x4 cameraMatrix = Matrix4x4::MakeAffine(
        Vector3{ 1, 1, 1 },          
        worldTransform_.rotationQuaternion_,
        cameraWorldPosition_
    );

    // ワールド行列の逆行列がビュー行列
    return Matrix4x4::Inverse(cameraMatrix);
}