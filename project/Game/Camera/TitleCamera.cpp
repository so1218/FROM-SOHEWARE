#include "pch.h"
#include "TitleCamera.h"
#include "FollowCamera.h"
#include "TimeManager.h"

using namespace FS;

namespace
{
    constexpr float kDegToRad = Math::PI / 180.0f;

    // 始めと終わりがなめらかに止まる 0〜1 の補間
    float SmootherStep(float t)
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }
}

TitleCamera::TitleCamera(Engine* engine, FollowCamera* followCamera)
    : followCamera_(followCamera)
{
    binder_ = std::make_unique<PropertyBinder>(engine, "TitleCamera");
}

void TitleCamera::Initialize()
{
    binder_->Bind("Position", &position_, { 0.0f, 20.0f, -60.0f }, 0.1f);
    binder_->Bind("Rotation", &rotationDegrees_, { 10.0f, 0.0f, 0.0f }, 0.1f);
    binder_->Bind("Fov", &fov_, 0.45f, 0.005f, 0.05f, 3.0f);
    binder_->Bind("SwayAmount", &swayAmount_, 0.3f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("SwaySpeed", &swaySpeed_, 0.4f, 0.01f, 0.0f, 10.0f);
    binder_->Bind("TransitionTime", &transitionTime_, 2.5f, 0.05f, 0.1f, 10.0f);
    binder_->Bind("ArcHeight", &arcHeight_, 4.0f, 0.1f, 0.0f, 100.0f);
}

void TitleCamera::UpdateCamera(Camera* camera)
{
    elapsedTime_ += TimeManager::GetInstance()->GetDeltaTime();

    if (isTransitioning_)
    {
        UpdateTransition(camera);
    }
    else
    {
        UpdateTitlePose(camera);
    }
}

void TitleCamera::UpdateTitlePose(Camera* camera)
{
    const Vector3 sway = {
        std::sin(elapsedTime_ * swaySpeed_ * 1.3f) * swayAmount_,
        std::sin(elapsedTime_ * swaySpeed_ * 1.7f) * swayAmount_ * 0.6f,
        0.0f
    };
    const Vector3 swayRotation = { sway.y * 0.5f * kDegToRad, sway.x * 0.5f * kDegToRad, 0.0f };

    camera->SetTranslation(position_ + sway);
    camera->SetRotation(Quaternion::QuaternionFromEuler(rotationDegrees_ * kDegToRad + swayRotation));
    camera->SetFov(fov_);
}

void TitleCamera::UpdateTransition(Camera* camera)
{
    transitionTimer_ = std::min(transitionTimer_ + TimeManager::GetInstance()->GetDeltaTime(), transitionTime_);
    const float progress = transitionTimer_ / transitionTime_;
    const float blend = SmootherStep(progress);

    // 行き先は毎フレーム計算
    followCamera_->UpdateCamera(camera);
    const Vector3 endPosition = camera->GetTranslation();
    const Quaternion endRotation = camera->GetRotation();
    const float endFov = camera->GetFov();

    Vector3 position = startPosition_ + (endPosition - startPosition_) * blend;
    position.y += std::sin(progress * Math::PI) * arcHeight_;

    camera->SetTranslation(position);
    camera->SetRotation(Quaternion::Slerp(startRotation_, endRotation, blend));
    camera->SetFov(startFov_ + (endFov - startFov_) * blend);
}

void TitleCamera::StartTransition(Camera* camera)
{
    startPosition_ = camera->GetTranslation();
    startRotation_ = camera->GetRotation();
    startFov_ = camera->GetFov();

    // FollowCamera の補間を落ち着いた状態から始める
    followCamera_->PlaceBehindTarget();
    followCamera_->Reset(camera);

    isTransitioning_ = true;
    transitionTimer_ = 0.0f;
}

void TitleCamera::DrawProperties()
{
    binder_->Draw("Position", "位置");
    binder_->Draw("Rotation", "向き (度)");
    binder_->Draw("Fov", "画角");
    binder_->Draw("SwayAmount", "揺れの大きさ");
    binder_->Draw("SwaySpeed", "揺れの速さ");
    binder_->Draw("TransitionTime", "プレイヤーに寄る時間 (秒)");
    binder_->Draw("ArcHeight", "寄る途中で持ち上げる高さ");
}
