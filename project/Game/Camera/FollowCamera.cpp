#include "pch.h"
#include "FollowCamera.h"
#include "Player.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "TimeManager.h"

using namespace FE;

FollowCamera::FollowCamera(FE::Engine* engine, const FE::WorldTransform* target)
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

    // エイム時パラメータ
    binder_->Bind("Aim Distance", &aimDistance_, 18.0f, 0.1f, 1.0f, 100.0f);
    binder_->Bind("Aim LookAt Offset", &aimLookAtOffset_, { 0.0f, 1.4f, 0.0f });
    binder_->Bind("Aim Shoulder Offset", &aimShoulderOffset_, { 1.8f, 0.1f, 0.0f });
    binder_->Bind("Aim FOV", &aimFov_, 0.35f, 0.01f, 0.1f, 1.5f);
    binder_->Bind("Aim Transition Time", &aimTransitionSmoothTime_, 0.12f, 0.01f, 0.01f, 1.0f);

    // カメラ回転・速度設定
    binder_->Bind("Rotate Speed Yaw", &rotateSpeedYaw_, 2.0f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("Rotate Speed Pitch", &rotateSpeedPitch_, 2.0f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("Rotation Smooth Time", &rotationSmoothTime_, 0.1f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("Position Lerp Speed", &positionLerpSpeed_, 8.0f, 0.1f, 0.0f, 20.0f);

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

    // 右スティックのアナログ生値（-32768 ~ 32767）を取得
    SHORT rawRx = Input::GetInstance().GetRightStickX(0);
    SHORT rawRy = Input::GetInstance().GetRightStickY(0);

    // ★ エイム中と通常時でデッドゾーンの閾値を切り替える
    // ※ 生値 32768 に対して、エイム中は小さめ(2500)、通常時は標準(6000)
    float currentDeadzone = isAiming_ ? 2500.0f : 6000.0f;

    // リマップ付きデッドゾーン計算用ラムダ関数
    auto applyScaledDeadzone = [](SHORT rawVal, float deadzone) -> float {
        float val = static_cast<float>(rawVal);
        float absVal = std::abs(val);

        if (absVal <= deadzone)
        {
            return 0.0f;
        }

        // デッドゾーンを超えた範囲 (deadzone ~ 32767) を (0.0 ~ 1.0) に滑らかに変換
        float sign = (val > 0.0f) ? 1.0f : -1.0f;
        float normalized = (absVal - deadzone) / (32767.0f - deadzone);

        return sign * std::clamp(normalized, 0.0f, 1.0f);
        };

    rx = applyScaledDeadzone(rawRx, currentDeadzone);
    ry = applyScaledDeadzone(rawRy, currentDeadzone);

    // キーボード入力の加算（矢印キーなど）
    if (Input::GetInstance().IsKeyPressed(DIK_LEFT))  rx -= 1.0f;
    if (Input::GetInstance().IsKeyPressed(DIK_RIGHT)) rx += 1.0f;
    if (Input::GetInstance().IsKeyPressed(DIK_UP))    ry += 1.0f;
    if (Input::GetInstance().IsKeyPressed(DIK_DOWN))  ry -= 1.0f;

    // スティックの倒し幅（rx, ry）に応じて角度を更新
    float currentRotateSpeedYaw = isAiming_ ? rotateSpeedYaw_ * 0.6f : rotateSpeedYaw_;
    float currentRotateSpeedPitch = isAiming_ ? rotateSpeedPitch_ * 0.6f : rotateSpeedPitch_;

    targetYaw_ += rx * currentRotateSpeedYaw * dt;
    targetPitch_ -= ry * currentRotateSpeedPitch * dt;

    targetPitch_ = std::clamp(targetPitch_, minPitch_, maxPitch_);
    currentYaw_ = SmoothDampAngle(currentYaw_, targetYaw_, yawVelocity_, rotationSmoothTime_, dt);
    currentPitch_ = SmoothDamp(currentPitch_, targetPitch_, pitchVelocity_, rotationSmoothTime_, dt);

    // ---------------------------------------------------------
    // ターゲット位置のスムージングとワールド座標計算
    // ---------------------------------------------------------
    Vector3 actualPlayerPos = target_->translation_;
    float posEffectiveSpeed = Math::MyMin<float>(1.0f, positionLerpSpeed_ * dt);
    smoothedTargetPos_ = Vector3::Lerp(smoothedTargetPos_, actualPlayerPos, posEffectiveSpeed);

    // Yaw回転に合わせて肩越しオフセットをワールド座標系に変換
    Quaternion yawRotation = Quaternion::QuaternionFromEuler({ 0.0f, currentYaw_, 0.0f });
    Vector3 rotatedShoulderOffset = yawRotation.RotateVector(currentShoulderOffset_);

    // カメラの球面オフセット
    float horizontalDistance = std::cos(currentPitch_) * distance_;
    Vector3 sphereOffset = {
        std::sin(currentYaw_) * horizontalDistance,
        std::sin(currentPitch_) * distance_,
        std::cos(currentYaw_) * horizontalDistance
    };

    // プレイヤーの旋回中心
    Vector3 centerPos = smoothedTargetPos_ + currentLookAtOffset_;

    // 最終的なカメラ位置と注視点
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

    // 最終的な行列生成と適用
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
    }

    if (ImGui::CollapsingHeader("エイム時カメラ設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Aim Distance", "エイム時距離");
        binder_->Draw("Aim Shoulder Offset", "肩越しオフセット");
        binder_->Draw("Aim LookAt Offset", "エイム時注視点");
        binder_->Draw("Aim FOV", "エイム時FOV");
        binder_->Draw("Aim Transition Time", "エイム切替スピード");
    }

    if (ImGui::CollapsingHeader("操作感度・制限"))
    {
        binder_->Draw("Rotate Speed Yaw", "左右回転速度");
        binder_->Draw("Rotate Speed Pitch", "上下回転速度");
        binder_->Draw("Position Lerp Speed", "追従スピード");
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

    smoothedTargetPos_ = target_->GetWorldPosition();

    yawVelocity_ = pitchVelocity_ = distanceVelocity_ = fovVelocity_ = 0.0f;
    shoulderOffsetVelocity_ = { 0.0f, 0.0f, 0.0f };
    lookAtOffsetVelocity_ = { 0.0f, 0.0f, 0.0f };
    posVelocity_ = { 0.0f, 0.0f, 0.0f };

    UpdateCamera(camera);
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