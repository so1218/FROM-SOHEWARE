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

    // 初期角度・距離設定
    currentYaw_ = targetYaw_ = PI;
    currentPitch_ = targetPitch_ = 0.3f;
    distance_ = targetDistance_ = 50.0f;

    // 速度とスムーズ時間の初期化
    yawVelocity_ = pitchVelocity_ = distanceVelocity_ = 0.0f;
    rotationSmoothTime_ = 0.1f;
    zoomSmoothTime_ = 0.2f;

    // 初期位置計算
    Vector3 targetPos = target_->GetWorldTransform().translation_;
    float horizontalDistance = std::cos(currentPitch_) * distance_;
    Vector3 targetOffset = {
        std::sin(currentYaw_) * horizontalDistance,
        std::sin(currentPitch_) * distance_,
        std::cos(currentYaw_) * horizontalDistance
    };

    smoothedTargetPos_ = targetPos;
    posVelocity_ = { 0.0f, 0.0f, 0.0f };

    Vector3 cameraPos = targetPos + targetOffset;
    Vector3 cameraTarget = targetPos + lookAtOffset_;
    Vector3 cameraUp = { 0.0f, 1.0f, 0.0f };
    Vector3 cameraForward = (cameraTarget - cameraPos).Normalize();
    Quaternion cameraRot = Quaternion::LookRotation(cameraForward, cameraUp);

    camera_->SetTranslation(cameraPos);
    camera_->SetRotation(cameraRot);
    currentCameraRot_ = cameraRot;
}

void FollowCamera::Update()
{
    if (!target_ || !camera_) return;

    float dt = TimeManager::GetInstance()->GetUnscaledDeltaTime();
    shakeEffect_.Update();

    const float rotateSpeed = 2.0f;
    const float zoomSpeed = 20.0f;

    // 回転入力
    if (Input::GetInstance().IsKeyPressed(DIK_LEFT))
        targetYaw_ += rotateSpeed * dt;
    else if (Input::GetInstance().IsKeyPressed(DIK_RIGHT))
        targetYaw_ -= rotateSpeed * dt;

    // ズーム入力
    if (Input::GetInstance().IsKeyPressed(DIK_UP))
        targetDistance_ -= zoomSpeed * dt;
    else if (Input::GetInstance().IsKeyPressed(DIK_DOWN))
        targetDistance_ += zoomSpeed * dt;

    // 回転・ズーム補間
    targetDistance_ = std::clamp(targetDistance_, minDistance_, maxDistance_);
    distance_ = SmoothDamp(distance_, targetDistance_, distanceVelocity_, zoomSmoothTime_, dt);

    targetPitch_ = std::clamp(targetPitch_, minPitch_, maxPitch_);
    currentYaw_ = SmoothDampAngle(currentYaw_, targetYaw_, yawVelocity_, rotationSmoothTime_, dt);
    currentPitch_ = SmoothDamp(currentPitch_, targetPitch_, pitchVelocity_, rotationSmoothTime_, dt);

    // ターゲット位置のスムージング
    Vector3 actualPlayerPos = target_->GetWorldTransform().translation_;
    float posEffectiveSpeed = MyMin<float>(1.0f, positionLerpSpeed_ * dt);
    smoothedTargetPos_ = Vector3::Lerp(smoothedTargetPos_, actualPlayerPos, posEffectiveSpeed);

    // カメラ位置計算
    float horizontalDistance = std::cos(currentPitch_) * distance_;
    Vector3 targetOffset = {
        std::sin(currentYaw_) * horizontalDistance,
        std::sin(currentPitch_) * distance_,
        std::cos(currentYaw_) * horizontalDistance
    };

    Vector3 targetPos = smoothedTargetPos_;
    Vector3 finalCameraPos = targetPos + targetOffset;
    Vector3 desiredCameraTarget = actualPlayerPos + lookAtOffset_;

    // カメラ回転とシェイク適用
    Vector3 finalCameraForward = (desiredCameraTarget - finalCameraPos).Normalize();
    currentCameraRot_ = Quaternion::LookRotation(finalCameraForward, { 0.0f, 1.0f, 0.0f });
    Vector3 shakeOffset = shakeEffect_.GetOffset();

    camera_->SetTranslation(finalCameraPos + shakeOffset);
    camera_->SetRotation(currentCameraRot_);
    camera_->UpdateViewProjectionMatrix();
}

void FollowCamera::DebugDraw()
{
    ImGui::Begin("FollowCamera");

    ImGui::Text("--- Target Values ---");
    ImGui::DragFloat("Yaw", &targetYaw_, 0.01f);
    ImGui::DragFloat("Pitch", &targetPitch_, 0.01f, minPitch_, maxPitch_);
    ImGui::DragFloat("Distance", &targetDistance_, 0.1f, minDistance_, maxDistance_);

    ImGui::Text("--- Current Values ---");
    ImGui::Text("Yaw: %.2f", currentYaw_);
    ImGui::Text("Pitch: %.2f", currentPitch_);
    ImGui::Text("Distance: %.2f", distance_);

    ImGui::Separator();
    ImGui::Text("--- Smooth Settings ---");
    ImGui::DragFloat("Rotation Smooth Time", &rotationSmoothTime_, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Zoom Smooth Time", &zoomSmoothTime_, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Position Lerp Speed", &positionLerpSpeed_, 0.1f, 0.0f, 20.0f);

    ImGui::End();
}

void FollowCamera::StartShake(float duration, float intensity)
{
    shakeEffect_.Start(duration, intensity);
}

float FollowCamera::SmoothDamp(float current, float target, float& currentVelocity,
    float smoothTime, float deltaTime, float maxSpeed)
{
    smoothTime = std::max(0.0001F, smoothTime);
    float omega = 2.0F / smoothTime;
    float x = omega * deltaTime;
    float exp = 1.0F / (1.0F + x + 0.48F * x * x + 0.235F * x * x * x);
    float change = current - target;
    float originalTo = target;

    float maxChange = maxSpeed * smoothTime;
    change = std::clamp(change, -maxChange, maxChange);
    target = current - change;

    float temp = (currentVelocity + omega * change) * deltaTime;
    currentVelocity = (currentVelocity - omega * temp) * exp;
    float output = target + (change + temp) * exp;

    // オーバーシュート防止
    if ((originalTo - current > 0.0F) == (output > originalTo))
    {
        output = originalTo;
        currentVelocity = (output - originalTo) / deltaTime;
    }
    return output;
}

float FollowCamera::SmoothDampAngle(float current, float target, float& currentVelocity,
    float smoothTime, float deltaTime, float maxSpeed)
{
    float delta = target - current;
    while (delta > PI) delta -= PI * 2.0f;
    while (delta < -PI) delta += PI * 2.0f;
    target = current + delta;
    return SmoothDamp(current, target, currentVelocity, smoothTime, deltaTime, maxSpeed);
}