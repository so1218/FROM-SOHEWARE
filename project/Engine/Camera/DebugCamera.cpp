#include "pch.h"
#include "DebugCamera.h"
#include "Input.h"
#include "ImGuiManager.h"

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

#ifdef IS_DEVELOPMENT
    isEnabled_ = true;
#else
    isEnabled_ = false;
#endif

    // ワールド行列の更新
    worldTransform_.UpdateMatrix();
}

void DebugCamera::Update()
{
    if (!isEnabled_) return;

    bool canInput = true;
#ifdef IS_DEVELOPMENT
    // マウスがScene Viewの上にないなら操作しない
    // Gizmoをドラッグ中ならカメラ操作はしない
    if (!ImGuiManager::IsSceneHovered() || ImGuizmo::IsUsing())
    {
        canInput = false;
    }
#endif

    if (canInput)
    {
        // ズームはマウスホイールで操作
        {
            int wheelDelta = Input::GetInstance().GetMouseWheelDelta();
            distance_ -= wheelDelta * zoomSpeed_;

            Quaternion currentRotation = worldTransform_.rotationQuaternion_;
            Vector3 forward = currentRotation.RotateVector(Vector3(0.0f, 0.0f, 1.0f));
            if (distance_ < minDistance_) {
                // targetをforward方向に押す
                target_ += forward * (minDistance_ - distance_);
                distance_ = minDistance_;
            }
        }

        // マウス中ボタンドラッグ時の処理
        if (Input::GetInstance().IsMouseButtonPressed(Input::MouseButton::Middle))
        {
            // マウスの移動量を取得
            int deltaX = Input::GetInstance().GetMouseState().lX;
            int deltaY = Input::GetInstance().GetMouseState().lY;

            // Shiftキーの有無で分岐
            if (Input::GetInstance().IsKeyPressed(DIK_LSHIFT))
            {
                // 平行移動
                Quaternion currentRotation = worldTransform_.rotationQuaternion_;
                Vector3 right = currentRotation.RotateVector(Vector3(1.0f, 0.0f, 0.0f));
                Vector3 up = currentRotation.RotateVector(Vector3(0.0f, 1.0f, 0.0f));

                target_ -= right * static_cast<float>(deltaX) * dragSpeed_;
                target_ += up * static_cast<float>(deltaY) * dragSpeed_;
            }
            else
            {
                // 回転
                currentYaw_ += deltaX * rotateSpeed_;
                currentPitch_ += deltaY * rotateSpeed_;
            }
        }
    }

    // currentPitch_とcurrentYaw_を使って回転クォータニオンを作成し、常に反映
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