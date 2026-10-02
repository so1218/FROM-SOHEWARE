#include "pch.h"
#include "FollowCamera.h"
#include "Player.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "TimeManager.h"

using namespace FE;

FollowCamera::FollowCamera(Engine* engine, const WorldTransform* target)
    : target_(target)
{
    engine_ = engine;
}

void FollowCamera::Initialize()
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "FollowCamera");

    // 通常時パラメータ
    binder_->Bind("Normal Distance", &normalDistance_, 45.0f, 0.1f, 5.0f, 200.0f);
    binder_->Bind("Normal LookAt Offset", &normalLookAtOffset_, { 0.0f, 1.5f, 0.0f });
    binder_->Bind("Normal FOV", &normalFov_, 0.45f, 0.01f, 0.1f, 1.5f);
    binder_->Bind("Normal Deadzone", &normalDeadzone_, 6000.0f, 100.0f, 0.0f, 15000.0f);

    // エイム時パラメータ
    binder_->Bind("Aim Distance", &aimDistance_, 18.0f, 0.1f, 1.0f, 100.0f);
    binder_->Bind("Aim LookAt Offset", &aimLookAtOffset_, { 0.0f, 1.4f, 0.0f });
    binder_->Bind("Aim Shoulder Offset", &aimShoulderOffset_, { 1.8f, 0.1f, 0.0f });
    binder_->Bind("Aim FOV", &aimFov_, 0.35f, 0.01f, 0.1f, 1.5f);
    binder_->Bind("Aim Transition Time", &aimTransitionSmoothTime_, 0.12f, 0.01f, 0.01f, 1.0f);
    binder_->Bind("Aim Deadzone", &aimDeadzone_, 2500.0f, 100.0f, 0.0f, 15000.0f);
    binder_->Bind("Aim Sens Multiplier", &aimRotateSpeedMultiplier_, 0.6f, 0.05f, 0.1f, 2.0f);

    // カメラ回転・速度・限界値設定
    binder_->Bind("Rotate Speed Yaw", &rotateSpeedYaw_, 2.0f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("Rotate Speed Pitch", &rotateSpeedPitch_, 2.0f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("Rotation Smooth Time", &rotationSmoothTime_, 0.1f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("Position Lerp Speed", &positionLerpSpeed_, 8.0f, 0.1f, 0.0f, 20.0f);
    binder_->Bind("Min Pitch", &minPitch_, -0.8f, 0.05f, -1.5f, 0.0f);
    binder_->Bind("Max Pitch", &maxPitch_, 1.4f, 0.05f, 0.0f, 1.5f);

    // リコイル設定
    binder_->Bind("Recoil Recovery Speed", &recoilRecoverySpeed_, 12.0f, 0.5f, 1.0f, 50.0f);

    // 地面補正設定
    binder_->Bind("Min Ground Offset", &minGroundOffset_, 0.8f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("Ground Check Radius", &groundCheckRadius_, 0.4f, 0.05f, 0.1f, 2.0f);

    // 初期状態の設定
    currentYaw_ = targetYaw_;
    currentPitch_ = targetPitch_;
    distance_ = normalDistance_;
    currentFov_ = normalFov_;
    currentLookAtOffset_ = normalLookAtOffset_;
    currentShoulderOffset_ = normalShoulderOffset_;

    yawVelocity_ = pitchVelocity_ = distanceVelocity_ = fovVelocity_ = 0.0f;
    shoulderOffsetVelocity_ = { 0.0f, 0.0f, 0.0f };
    lookAtOffsetVelocity_ = { 0.0f, 0.0f, 0.0f };

    if (target_)
    {
        smoothedTargetPos_ = target_->GetWorldPosition();
    }
}

void FollowCamera::UpdateCamera(Camera* camera)
{
    if (!target_ || !camera) return;

    float dt = TimeManager::GetInstance()->GetUnscaledDeltaTime();

    // エイム状態に応じた目標値の設定
    float activeTargetDistance = isAiming_ ? aimDistance_ : normalDistance_;
    Vector3 activeTargetShoulder = isAiming_ ? aimShoulderOffset_ : normalShoulderOffset_;
    Vector3 activeTargetLookAt = isAiming_ ? aimLookAtOffset_ : normalLookAtOffset_;
    float activeTargetFov = isAiming_ ? aimFov_ : normalFov_;

    // 目標値へスムーズ補間
    targetDistance_ = std::clamp(activeTargetDistance, minDistance_, maxDistance_);
    distance_ = SmoothDamp(distance_, targetDistance_, distanceVelocity_, aimTransitionSmoothTime_, dt);

    currentShoulderOffset_.x = SmoothDamp(currentShoulderOffset_.x, activeTargetShoulder.x, shoulderOffsetVelocity_.x, aimTransitionSmoothTime_, dt);
    currentShoulderOffset_.y = SmoothDamp(currentShoulderOffset_.y, activeTargetShoulder.y, shoulderOffsetVelocity_.y, aimTransitionSmoothTime_, dt);
    currentShoulderOffset_.z = SmoothDamp(currentShoulderOffset_.z, activeTargetShoulder.z, shoulderOffsetVelocity_.z, aimTransitionSmoothTime_, dt);

    currentLookAtOffset_.x = SmoothDamp(currentLookAtOffset_.x, activeTargetLookAt.x, lookAtOffsetVelocity_.x, aimTransitionSmoothTime_, dt);
    currentLookAtOffset_.y = SmoothDamp(currentLookAtOffset_.y, activeTargetLookAt.y, lookAtOffsetVelocity_.y, aimTransitionSmoothTime_, dt);
    currentLookAtOffset_.z = SmoothDamp(currentLookAtOffset_.z, activeTargetLookAt.z, lookAtOffsetVelocity_.z, aimTransitionSmoothTime_, dt);

    currentFov_ = SmoothDamp(currentFov_, activeTargetFov, fovVelocity_, aimTransitionSmoothTime_, dt);
    camera->SetFov(currentFov_);

    // ---------------------------------------------------------
    // 入力によるカメラ回転処理（右スティック 360度アナログ対応）
    // ---------------------------------------------------------
    float rx = 0.0f;
    float ry = 0.0f;

    SHORT rawRx = Input::GetInstance().GetRightStickX(0);
    SHORT rawRy = Input::GetInstance().GetRightStickY(0);

    // バインドされたデッドゾーンを使用
    float currentDeadzone = isAiming_ ? aimDeadzone_ : normalDeadzone_;

    auto applyScaledDeadzone = [](SHORT rawVal, float deadzone) -> float {
        float val = static_cast<float>(rawVal);
        float absVal = std::abs(val);

        if (absVal <= deadzone)
        {
            return 0.0f;
        }

        float sign = (val > 0.0f) ? 1.0f : -1.0f;
        float normalized = (absVal - deadzone) / (kMaxStickValue - deadzone);

        return sign * std::clamp(normalized, 0.0f, 1.0f);
        };

    rx = applyScaledDeadzone(rawRx, currentDeadzone);
    ry = applyScaledDeadzone(rawRy, currentDeadzone);

    // キーボード入力の加算
    if (Input::GetInstance().IsKeyPressed(DIK_LEFT))  rx -= 1.0f;
    if (Input::GetInstance().IsKeyPressed(DIK_RIGHT)) rx += 1.0f;
    if (Input::GetInstance().IsKeyPressed(DIK_UP))    ry += 1.0f;
    if (Input::GetInstance().IsKeyPressed(DIK_DOWN))  ry -= 1.0f;

    // バインドされた倍率を使用
    float currentRotateSpeedYaw = isAiming_ ? rotateSpeedYaw_ * aimRotateSpeedMultiplier_ : rotateSpeedYaw_;
    float currentRotateSpeedPitch = isAiming_ ? rotateSpeedPitch_ * aimRotateSpeedMultiplier_ : rotateSpeedPitch_;

    targetYaw_ += rx * currentRotateSpeedYaw * dt;
    targetPitch_ -= ry * currentRotateSpeedPitch * dt;

    targetPitch_ = std::clamp(targetPitch_, minPitch_, maxPitch_);
    currentYaw_ = SmoothDampAngle(currentYaw_, targetYaw_, yawVelocity_, rotationSmoothTime_, dt);
    currentPitch_ = SmoothDamp(currentPitch_, targetPitch_, pitchVelocity_, rotationSmoothTime_, dt);

    // ---------------------------------------------------------
    // 反動オフセットの復元
    // ---------------------------------------------------------
    recoilPitch_ = Math::Lerp(recoilPitch_, 0.0f, recoilRecoverySpeed_ * dt);
    recoilYaw_ = Math::Lerp(recoilYaw_, 0.0f, recoilRecoverySpeed_ * dt);

    float finalYaw = currentYaw_ + recoilYaw_;
    float finalPitch = currentPitch_ + recoilPitch_;

    // ---------------------------------------------------------
    // ワールド座標計算
    // ---------------------------------------------------------
    Vector3 actualPlayerPos = target_->translation_;
    float posEffectiveSpeed = Math::MyMin<float>(1.0f, positionLerpSpeed_ * dt);
    smoothedTargetPos_ = Vector3::Lerp(smoothedTargetPos_, actualPlayerPos, posEffectiveSpeed);

    Quaternion yawRotation = Quaternion::QuaternionFromEuler({ 0.0f, finalYaw, 0.0f });
    Vector3 rotatedShoulderOffset = yawRotation.RotateVector(currentShoulderOffset_);

    float horizontalDistance = std::cos(finalPitch) * distance_;
    Vector3 sphereOffset = {
        std::sin(finalYaw) * horizontalDistance,
        std::sin(finalPitch) * distance_,
        std::cos(finalYaw) * horizontalDistance
    };

    Vector3 centerPos = smoothedTargetPos_ + currentLookAtOffset_;

    Vector3 desiredCameraTarget = centerPos + rotatedShoulderOffset;
    Vector3 finalCameraPos = centerPos + sphereOffset + rotatedShoulderOffset;

    // 地面埋まり防止処理
    if (terrain_)
    {
        float maxTerrainHeight = -FLT_MAX;
        float h = 0.0f;

        Vector2 checkOffsets[] = {
            { 0.0f, 0.0f },
            { groundCheckRadius_, 0.0f },
            { -groundCheckRadius_, 0.0f },
            { 0.0f, groundCheckRadius_ },
            { 0.0f, -groundCheckRadius_ }
        };

        for (const auto& offset : checkOffsets)
        {
            if (terrain_->GetHeightAt(finalCameraPos.x + offset.x, finalCameraPos.z + offset.y, h))
            {
                maxTerrainHeight = std::max(maxTerrainHeight, h);
            }
        }

        if (maxTerrainHeight != -FLT_MAX)
        {
            float minCamY = maxTerrainHeight + minGroundOffset_;
            if (finalCameraPos.y < minCamY)
            {
                finalCameraPos.y = minCamY;
            }
        }
    }

    // 行列生成
    Vector3 finalCameraForward = (desiredCameraTarget - finalCameraPos).Normalize();
    currentCameraRot_ = Quaternion::LookRotation(finalCameraForward, { 0.0f, 1.0f, 0.0f });

    camera->SetTranslation(finalCameraPos);
    camera->SetRotation(currentCameraRot_);
}

void FollowCamera::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::Begin("追従カメラ");

    if (ImGui::CollapsingHeader("通常時カメラ設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Normal Distance", "通常時距離");
        binder_->Draw("Normal LookAt Offset", "通常時注視点オフセット");
        binder_->Draw("Normal FOV", "通常時FOV");
        binder_->Draw("Normal Deadzone", "通常時デッドゾーン");
    }

    if (ImGui::CollapsingHeader("エイム時カメラ設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Aim Distance", "エイム時距離");
        binder_->Draw("Aim Shoulder Offset", "肩越しオフセット");
        binder_->Draw("Aim LookAt Offset", "エイム時注視点");
        binder_->Draw("Aim FOV", "エイム時FOV");
        binder_->Draw("Aim Transition Time", "エイム切替スピード");
        binder_->Draw("Aim Deadzone", "エイム時デッドゾーン");
        binder_->Draw("Aim Sens Multiplier", "エイム時感度倍率");
    }

    if (ImGui::CollapsingHeader("リコイル設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Recoil Recovery Speed", "反動復元速度");
    }

    if (ImGui::CollapsingHeader("操作感度・制限"))
    {
        binder_->Draw("Rotate Speed Yaw", "左右回転速度");
        binder_->Draw("Rotate Speed Pitch", "上下回転速度");
        binder_->Draw("Position Lerp Speed", "追従スピード");
        binder_->Draw("Min Pitch", "最小ピッチ(下限)");
        binder_->Draw("Max Pitch", "最大ピッチ(上限)");
    }

    if (ImGui::CollapsingHeader("地形衝突判定"))
    {
        binder_->Draw("Min Ground Offset", "地面からの最低高度");
        binder_->Draw("Ground Check Radius", "地面判定半径");
    }

    ImGui::End();
#endif
}

void FollowCamera::Reset(Camera* camera)
{
    if (!target_ || !camera) return;

    currentYaw_ = targetYaw_;
    currentPitch_ = targetPitch_;
    distance_ = isAiming_ ? aimDistance_ : normalDistance_;
    currentFov_ = isAiming_ ? aimFov_ : normalFov_;
    currentShoulderOffset_ = isAiming_ ? aimShoulderOffset_ : normalShoulderOffset_;
    currentLookAtOffset_ = isAiming_ ? aimLookAtOffset_ : normalLookAtOffset_;

    // ★ 反動オフセットもリセット
    recoilPitch_ = 0.0f;
    recoilYaw_ = 0.0f;

    smoothedTargetPos_ = target_->GetWorldPosition();

    yawVelocity_ = pitchVelocity_ = distanceVelocity_ = fovVelocity_ = 0.0f;
    shoulderOffsetVelocity_ = { 0.0f, 0.0f, 0.0f };
    lookAtOffsetVelocity_ = { 0.0f, 0.0f, 0.0f };
    posVelocity_ = { 0.0f, 0.0f, 0.0f };

    UpdateCamera(camera);
}

void FollowCamera::AddRecoil(float pitchAmount, float yawAmount)
{
    // 照準を上に跳ね上げるためマイナス方向へ加算
    recoilPitch_ -= pitchAmount;

    // 左右のランダムブレを加える
    float randYaw = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * yawAmount;
    recoilYaw_ += randYaw;
}

float FollowCamera::SmoothDamp(float current, float target, float& currentVelocity,
    float smoothTime, float deltaTime, float maxSpeed)
{
    smoothTime = std::max(0.0001f, smoothTime);
    float omega = 2.0f / smoothTime;
    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = current - target;
    float originalTo = target;

    float maxChange = maxSpeed * smoothTime;
    change = std::clamp(change, -maxChange, maxChange);
    target = current - change;

    float temp = (currentVelocity + omega * change) * deltaTime;
    currentVelocity = (currentVelocity - omega * temp) * exp;
    float output = target + (change + temp) * exp;

    // オーバーシュート防止
    if ((originalTo - current > 0.0f) == (output > originalTo))
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