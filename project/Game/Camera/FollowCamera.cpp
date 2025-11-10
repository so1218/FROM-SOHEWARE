#include "FollowCamera.h"
#include "Player.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "GlobalVariables.h"

void FollowCamera::Initialize(Camera* camera, Player* target)
{
    camera_ = camera;
    target_ = target;

    // --- GlobalVariables 登録 ---
    auto* gv = GlobalVariables::GetInstance();
    gv->CreateGroup(GetGlobalVariableGroupName());
    gv->LoadFiles();

    gv->AddItem(GetGlobalVariableGroupName(), "Target Yaw", targetYaw_);
    gv->AddItem(GetGlobalVariableGroupName(), "Target Pitch", targetPitch_);
    gv->AddItem(GetGlobalVariableGroupName(), "Target Distance", targetDistance_);
    gv->AddItem(GetGlobalVariableGroupName(), "Rotation Smooth Time", rotationSmoothTime_);
    gv->AddItem(GetGlobalVariableGroupName(), "Zoom Smooth Time", zoomSmoothTime_);
    gv->AddItem(GetGlobalVariableGroupName(), "Position Lerp Speed", positionLerpSpeed_);
    gv->AddItem(GetGlobalVariableGroupName(), "Min Pitch", minPitch_);
    gv->AddItem(GetGlobalVariableGroupName(), "Max Pitch", maxPitch_);
    gv->AddItem(GetGlobalVariableGroupName(), "Min Distance", minDistance_);
    gv->AddItem(GetGlobalVariableGroupName(), "Max Distance", maxDistance_);
    gv->AddItem(GetGlobalVariableGroupName(), "Rotate Speed Yaw", rotateSpeedYaw_);
    gv->AddItem(GetGlobalVariableGroupName(), "Rotate Speed Pitch", rotateSpeedPitch_);

    // 初期角度・距離設定
    currentYaw_ = targetYaw_ = Math::PI;
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

    ApplyGlobalVariables();
}

void FollowCamera::ApplyGlobalVariables()
{
    auto* gv = GlobalVariables::GetInstance();

    targetYaw_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Target Yaw");
    targetPitch_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Target Pitch");
    targetDistance_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Target Distance");

    rotationSmoothTime_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Rotation Smooth Time");
    zoomSmoothTime_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Zoom Smooth Time");
    positionLerpSpeed_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Position Lerp Speed");

    minPitch_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Min Pitch");
    maxPitch_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Max Pitch");
    minDistance_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Min Distance");
    maxDistance_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Max Distance");

    rotateSpeedYaw_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Rotate Speed Yaw");
    rotateSpeedPitch_ = gv->GetFloatValue(GetGlobalVariableGroupName(), "Rotate Speed Pitch");
}

void FollowCamera::Update()
{
    if (!target_ || !camera_) return;

    float dt = TimeManager::GetInstance()->GetUnscaledDeltaTime();
    shakeEffect_.Update();

    // 左右キーでカメラを回転
    if (Input::GetInstance().IsKeyPressed(DIK_LEFT) || Input::GetInstance().IsLeftOnStick(0, Input::RightStick))
    {
        targetYaw_ -= rotateSpeedYaw_ * dt;
    }
    if (Input::GetInstance().IsKeyPressed(DIK_RIGHT) || Input::GetInstance().IsRightOnStick(0, Input::RightStick))
    {
        targetYaw_ += rotateSpeedYaw_ * dt;
    }
    if (Input::GetInstance().IsKeyPressed(DIK_UP) || Input::GetInstance().IsUpOnStick(0, Input::RightStick))
    {
        targetPitch_ -= rotateSpeedPitch_ * dt;
    }
    if (Input::GetInstance().IsKeyPressed(DIK_DOWN) || Input::GetInstance().IsDownOnStick(0, Input::RightStick))
    {
        targetPitch_ += rotateSpeedPitch_ * dt;
    }

    // 回転・ズーム補間
    targetDistance_ = std::clamp(targetDistance_, minDistance_, maxDistance_);
    distance_ = SmoothDamp(distance_, targetDistance_, distanceVelocity_, zoomSmoothTime_, dt);

    targetPitch_ = std::clamp(targetPitch_, minPitch_, maxPitch_);
    currentYaw_ = SmoothDampAngle(currentYaw_, targetYaw_, yawVelocity_, rotationSmoothTime_, dt);
    currentPitch_ = SmoothDamp(currentPitch_, targetPitch_, pitchVelocity_, rotationSmoothTime_, dt);

    // ターゲット位置のスムージング
    Vector3 actualPlayerPos = target_->GetWorldTransform().translation_;
    float posEffectiveSpeed = Math::MyMin<float>(1.0f, positionLerpSpeed_ * dt);
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
    ImGui::Begin("追従カメラ");

    ImGui::Separator();
    ImGui::Text("スムージング設定");

    ImGui::Separator();
    ImGui::Text("初期カメラ設定");

    if (ImGui::DragFloat("初期ヨー角", &targetYaw_, 0.01f, -Math::PI, Math::PI))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Target Yaw", targetYaw_);
    }
    if (ImGui::DragFloat("初期ピッチ角", &targetPitch_, 0.01f, -1.57f, 1.57f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Target Pitch", targetPitch_);
    }

    if (ImGui::DragFloat("初期距離", &targetDistance_, 0.1f, minDistance_, maxDistance_))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Target Distance", targetDistance_);
    }

    if (ImGui::DragFloat("回転スムース時間", &rotationSmoothTime_, 0.01f, 0.0f, 1.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Rotation Smooth Time", rotationSmoothTime_);
    }
    if (ImGui::DragFloat("ズームスムース時間", &zoomSmoothTime_, 0.01f, 0.0f, 1.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Zoom Smooth Time", zoomSmoothTime_);
    }
    if (ImGui::DragFloat("位置補間スピード", &positionLerpSpeed_, 0.1f, 0.0f, 20.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Position Lerp Speed", positionLerpSpeed_);
    }

    if (ImGui::DragFloat("左右回転速度", &rotateSpeedYaw_, 0.01f, 0.0f, 10.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Rotate Speed Yaw", rotateSpeedYaw_);
    }
    if (ImGui::DragFloat("上下回転速度", &rotateSpeedPitch_, 0.01f, 0.0f, 10.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Rotate Speed Pitch", rotateSpeedPitch_);
    }

    ImGui::Separator();
    ImGui::Text("制限値");

    if (ImGui::DragFloat("ピッチ最小角度", &minPitch_, 0.01f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Min Pitch", minPitch_);
    }
    if (ImGui::DragFloat("ピッチ最大角度", &maxPitch_, 0.01f, 0.0f, 0.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Max Pitch", maxPitch_);
    }
    if (ImGui::DragFloat("最小距離", &minDistance_, 0.1f, 0.0f, 200.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Min Distance", minDistance_);
    }
    if (ImGui::DragFloat("最大距離", &maxDistance_, 0.1f, 0.0f, 200.0f))
    {
        GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "Max Distance", maxDistance_);
    }

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
    while (delta > Math::PI) delta -= Math::PI * 2.0f;
    while (delta < -Math::PI) delta += Math::PI * 2.0f;
    target = current + delta;
    return SmoothDamp(current, target, currentVelocity, smoothTime, deltaTime, maxSpeed);
}