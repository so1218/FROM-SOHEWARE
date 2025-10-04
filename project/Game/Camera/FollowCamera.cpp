#include "FollowCamera.h"
#include "Player.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "MathUtils.h"

void FollowCamera::Initialize(Camera* camera, Player* target) {
    camera_ = camera;
    target_ = target;
   
    yaw_ = PI; 
    distance_ = 50.0f; 
    height_ = 5.0f; 

    // 初期位置と回転を計算
    Vector3 targetPos = target_->GetWorldTransform().translation_;
    Vector3 cameraPos = targetPos + Vector3(std::sin(yaw_) * distance_, height_, std::cos(yaw_) * distance_);
    Vector3 cameraTarget = targetPos;
    Vector3 cameraUp = { 0.0f, 1.0f, 0.0f };
    Vector3 cameraForward = (cameraTarget - cameraPos).Normalize();
    Quaternion cameraRot = Quaternion::LookRotation(cameraForward, cameraUp);

    // 現在のカメラ位置と回転として設定
    currentCameraPos_ = cameraPos;
    currentCameraRot_ = cameraRot;

    interpSpeed_ = 0.35f; // 補間速度を初期化
}

void FollowCamera::Update()
{
    if (!target_ || !camera_) return;

    // ==== 入力処理 ====
    const float rotateSpeed = 0.02f; // 回転速度
    const float zoomSpeed = 0.1f;    // ズーム速度

    // 左右回転（Y軸回転）
    //if (Input::IsKeyPressed(DIK_A) || Input::IsLeftOnStick(0, Input::RightStick)) 
    //{
    //    yaw_ += rotateSpeed;
    //}
    //else if (Input::IsKeyPressed(DIK_D) || Input::IsRightOnStick(0, Input::RightStick))
    //{
    //    yaw_ -= rotateSpeed;
    //}

    //// ズームイン/アウト（距離調整）
    //if (Input::IsKeyPressed(DIK_W) || Input::IsUpOnStick(0, Input::RightStick))
    //{
    //    distance_ -= zoomSpeed;
    //    if (distance_ < 1.0f)
    //    {
    //        distance_ = 1.0f;
    //    } // 最小距離制限
    //}
    //else if (Input::IsKeyPressed(DIK_S) || Input::IsDownOnStick(0, Input::RightStick))
    //{
    //    distance_ += zoomSpeed;
    //}

    // ==== 目標オフセット計算 ====
    Vector3 targetOffset;
    targetOffset.x = std::sin(yaw_) * distance_;
    targetOffset.z = std::cos(yaw_) * distance_;
    targetOffset.y = height_;

    // ==== 目標カメラ位置と回転の計算 ====
    Vector3 targetPos = target_->GetWorldTransform().translation_;
    Vector3 desiredCameraPos = targetPos + targetOffset; // 目標のカメラ位置
    Vector3 desiredCameraTarget = targetPos;
    Vector3 cameraUp = { 0.0f, 1.0f, 0.0f };

    Vector3 desiredCameraForward = (desiredCameraTarget - desiredCameraPos).Normalize();
    Quaternion desiredCameraRot = Quaternion::LookRotation(desiredCameraForward, cameraUp); // 目標のカメラ回転

    // ==== 補間処理 ====
    // 現在のカメラ位置を目標位置へ線形補間
    currentCameraPos_ = Vector3::Lerp(currentCameraPos_, desiredCameraPos, interpSpeed_);
    // 現在のカメラ回転を目標回転へ球面線形補間 (Slerp)
    currentCameraRot_ = Quaternion::Slerp(currentCameraRot_, desiredCameraRot, interpSpeed_);

    // ==== カメラに設定 ====
    camera_->SetTranslation(currentCameraPos_);
    camera_->SetRotation(currentCameraRot_);
    camera_->UpdateViewProjectionMatrix();
}

// デバッグ描画処理
void FollowCamera::DebugDraw()
{
    ImGui::Begin("FollowCamera");
    ImGui::DragFloat("Yaw", &yaw_, 0.01f);
    ImGui::DragFloat("Distance", &distance_, 0.1f, 1.0f, 50.0f);
    ImGui::DragFloat("Height", &height_, 0.1f, -10.0f, 20.0f);
    ImGui::DragFloat("Interp Speed", &interpSpeed_, 0.001f, 0.0f, 1.0f); // 補間速度を調整可能に
    ImGui::End();
}