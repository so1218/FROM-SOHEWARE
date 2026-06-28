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
    // Binderの作成
    binder_ = std::make_unique<PropertyBinder>(engine_, "FollowCamera");

    binder_->Bind("Target Yaw", &targetYaw_, Math::PI, 0.01f, -Math::PI, Math::PI);
    binder_->Bind("Target Pitch", &targetPitch_, 0.3f, 0.01f, -1.57f, 1.57f);
    binder_->Bind("Target Distance", &targetDistance_, 50.0f, 0.1f, 5.0f, 200.0f);
    binder_->Bind("LookAt Offset", &lookAtOffset_, { 0.0f, 1.5f, 0.0f });
    binder_->Bind("Rotation Smooth Time", &rotationSmoothTime_, 0.1f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("Zoom Smooth Time", &zoomSmoothTime_, 0.2f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("Position Lerp Speed", &positionLerpSpeed_, 5.0f, 0.1f, 0.0f, 20.0f);
    binder_->Bind("Min Pitch", &minPitch_, -1.5f, 0.01f, -1.57f, 1.57f);
    binder_->Bind("Max Pitch", &maxPitch_, 1.5f, 0.01f, -1.57f, 1.57f);
    binder_->Bind("Min Distance", &minDistance_, 5.0f, 0.1f, 0.0f, 200.0f);
    binder_->Bind("Max Distance", &maxDistance_, 100.0f, 0.1f, 0.0f, 200.0f);
    binder_->Bind("Rotate Speed Yaw", &rotateSpeedYaw_, 2.0f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("Rotate Speed Pitch", &rotateSpeedPitch_, 2.0f, 0.01f, 0.0f, 10.0f);

    // 内部変数の初期値として適用
    currentYaw_ = targetYaw_;
    currentPitch_ = targetPitch_;
    distance_ = targetDistance_;

    // 速度初期化
    yawVelocity_ = pitchVelocity_ = distanceVelocity_ = 0.0f;

    // 初期位置計算（ターゲットが存在する場合）
    if (target_)
    {
        Vector3 targetPos = target_->GetWorldPosition();
        smoothedTargetPos_ = targetPos;

        float horizontalDistance = std::cos(currentPitch_) * distance_;
        Vector3 targetOffset = {
            std::sin(currentYaw_) * horizontalDistance,
            std::sin(currentPitch_) * distance_,
            std::cos(currentYaw_) * horizontalDistance
        };

        Vector3 cameraPos = targetPos + targetOffset;
        // 注視点はターゲット位置（オフセットがあれば加算）
        Vector3 cameraTarget = targetPos + lookAtOffset_;

        Vector3 cameraUp = { 0.0f, 1.0f, 0.0f };
        Vector3 cameraForward = (cameraTarget - cameraPos).Normalize();
        Quaternion cameraRot = Quaternion::LookRotation(cameraForward, cameraUp);

        currentCameraRot_ = cameraRot;
    }
}

void FollowCamera::UpdateCamera(Camera* camera)
{
    if (!target_ || !camera) return;

    float dt = TimeManager::GetInstance()->GetUnscaledDeltaTime();

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
    Vector3 actualPlayerPos = target_->GetWorldPosition();
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

    camera->SetTranslation(finalCameraPos);
    camera->SetRotation(currentCameraRot_);
}

void FollowCamera::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("追従カメラ");

    if (ImGui::CollapsingHeader("初期カメラ設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Target Yaw", "目標ヨー角");
        binder_->Draw("Target Pitch", "目標ピッチ角");
        binder_->Draw("Target Distance", "目標距離");
        binder_->Draw("LookAt Offset", "注視点オフセット");
    }

    if (ImGui::CollapsingHeader("スムージング設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Rotation Smooth Time", "回転スムース時間");
        binder_->Draw("Zoom Smooth Time", "ズームスムース時間");
        binder_->Draw("Position Lerp Speed", "位置補間スピード");
    }

    if (ImGui::CollapsingHeader("操作感度", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Rotate Speed Yaw", "左右回転速度");
        binder_->Draw("Rotate Speed Pitch", "上下回転速度");
    }

    if (ImGui::CollapsingHeader("制限設定"))
    {
        binder_->Draw("Min Pitch", "ピッチ最小角度");
        binder_->Draw("Max Pitch", "ピッチ最大角度");
        binder_->Draw("Min Distance", "最小距離");
        binder_->Draw("Max Distance", "最大距離");
    }

    ImGui::End();
#endif
}

void FollowCamera::Reset(Camera* camera)
{
    if (!target_ || !camera) return;

    // 補間中の値を全て目標値で上書き
    currentYaw_ = targetYaw_;
    currentPitch_ = targetPitch_;
    distance_ = targetDistance_;
    smoothedTargetPos_ = target_->GetWorldPosition();

    // 速度もゼロにリセットして、止める
    yawVelocity_ = 0.0f;
    pitchVelocity_ = 0.0f;
    distanceVelocity_ = 0.0f;
    posVelocity_ = { 0.0f, 0.0f, 0.0f };

    // その状態で一度計算を走らせてカメラに適用
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