#include "FollowCamera.h"
#include "Player.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "TimeManager.h"

void FollowCamera::Initialize(Camera* camera, Player* target) 
{
    camera_ = camera;
    target_ = target;
   
    yaw_ = PI; 
    distance_ = 50.0f; 
    targetDistance_ = 50.0f;
    pitch_ = 0.3f;
    interpSpeed_ = 8.0f;

    // 初期位置と回転を計算
    Vector3 targetPos = target_->GetWorldTransform().translation_;
    float horizontalDistance = std::cos(pitch_) * distance_;
    Vector3 targetOffset;
    targetOffset.x = std::sin(yaw_) * horizontalDistance;
    targetOffset.z = std::cos(yaw_) * horizontalDistance;
    targetOffset.y = std::sin(pitch_) * distance_;

    Vector3 cameraPos = targetPos + targetOffset; 
    Vector3 cameraTarget = targetPos + lookAtOffset_;
    Vector3 cameraUp = { 0.0f, 1.0f, 0.0f };
    Vector3 cameraForward = (cameraTarget - cameraPos).Normalize();
    Quaternion cameraRot = Quaternion::LookRotation(cameraForward, cameraUp);

    // 現在のカメラ位置と回転として設定
    currentCameraPos_ = cameraPos;
    currentCameraRot_ = cameraRot;

}

void FollowCamera::Update()
{
    if (!target_ || !camera_) return;

    float dt = TimeManager::GetInstance()->GetUnscaledDeltaTime();

    // 毎フレーム、シェイクタイマーを更新する
    shakeEffect_.Update();

    // 入力処理
    const float rotateSpeed = 2.0f; // 回転速度
    const float zoomSpeed = 20.0f;    // ズーム速度

    // 左右回転（Y軸回転）
    //if (Input::IsKeyPressed(DIK_A) || Input::IsLeftOnStick(0, Input::RightStick)) 
    //{
    //    yaw_ += rotateSpeed * dt;
    //}
    //else if (Input::IsKeyPressed(DIK_D) || Input::IsRightOnStick(0, Input::RightStick))
    //{
    //    yaw_ -= rotateSpeed * dt;
    //}

    //// ズームイン/アウト（距離調整）
    //if (Input::IsKeyPressed(DIK_W) || Input::IsUpOnStick(0, Input::RightStick))
    //{
    //    distance_ -= zoomSpeed * dt;
    //    if (distance_ < 1.0f)
    //    {
    //        distance_ = 1.0f;
    //    } // 最小距離制限
    //}
    //else if (Input::IsKeyPressed(DIK_S) || Input::IsDownOnStick(0, Input::RightStick))
    //{
    //    distance_ += zoomSpeed * dt;
    //}

    // 最小/最大距離制限
    targetDistance_ = std::clamp(targetDistance_, 5.0f, 100.0f);
    // 現在の距離を目標距離に補間する
    float zoomEffectiveSpeed = MyMin<float>(1.0f, zoomLerpSpeed_ * dt);
    distance_ = Lerp(distance_, targetDistance_, zoomEffectiveSpeed);

    // Pitchが上下反転しないようクランプする
    const float minPitch = -0.8f; 
    const float maxPitch = 1.4f;  
    pitch_ = std::clamp(pitch_, minPitch, maxPitch);

    // 目標オフセット計算
    Vector3 targetOffset;
    // 水平方向の距離
    float horizontalDistance = std::cos(pitch_) * distance_;

    targetOffset.x = std::sin(yaw_) * horizontalDistance; 
    targetOffset.z = std::cos(yaw_) * horizontalDistance;
    targetOffset.y = std::sin(pitch_) * distance_;

    // 目標カメラ位置と回転の計算
    Vector3 targetPos = target_->GetWorldTransform().translation_;
    Vector3 desiredCameraPos = targetPos + targetOffset; // 目標のカメラ位置
    Vector3 desiredCameraTarget = targetPos + lookAtOffset_;
    Vector3 cameraUp = { 0.0f, 1.0f, 0.0f };

    Vector3 desiredCameraForward = (desiredCameraTarget - desiredCameraPos).Normalize();
    Quaternion desiredCameraRot = Quaternion::LookRotation(desiredCameraForward, cameraUp); // 目標のカメラ回転

    // 1.0fを超えないようにしつつ、deltaTimeでスケーリング
    float effectiveSpeed = MyMin<float>(1.0f, interpSpeed_ * TimeManager::GetInstance()->GetUnscaledDeltaTime());

    // 補間処理
    // 現在のカメラ位置を目標位置へ線形補間
    currentCameraPos_ = Vector3::Lerp(currentCameraPos_, desiredCameraPos, effectiveSpeed);
    // 現在のカメラ回転を目標回転へ球面線形補間 
    currentCameraRot_ = Quaternion::Slerp(currentCameraRot_, desiredCameraRot, effectiveSpeed);

    // シェイクによるオフセットを取得
    Vector3 shakeOffset = shakeEffect_.GetOffset();

    // カメラに設定
    camera_->SetTranslation(currentCameraPos_ + shakeOffset);
    camera_->SetRotation(currentCameraRot_);
    camera_->UpdateViewProjectionMatrix();
}

// デバッグ描画処理
void FollowCamera::DebugDraw()
{
    ImGui::Begin("FollowCamera");
    ImGui::DragFloat("Yaw", &yaw_, 0.01f);
    ImGui::DragFloat("Distance", &distance_, 0.1f, 1.0f, 50.0f);
    ImGui::DragFloat("Interp Speed", &interpSpeed_, 0.001f, 0.0f, 1.0f); // 補間速度を調整可能に
    ImGui::End();
}

void FollowCamera::StartShake(float duration, float intensity)
{
    shakeEffect_.Start(duration, intensity);
}