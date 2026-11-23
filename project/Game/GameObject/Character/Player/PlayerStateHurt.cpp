#include "pch.h"
#include "PlayerStateHurt.h"
#include "PlayerStateNormal.h"
#include "PlayerStateJump.h"
#include "AnimationManager.h"
#include "TimeManager.h"

using namespace FS;

namespace
{
constexpr const char* kHitAnimation = "humanHit";
constexpr const char* kFallbackAnimation = "humanIdle";
constexpr float kBlendTime = 0.08f;
}

PlayerStateHurt::PlayerStateHurt(const Vector3& knockbackDirection)
    : direction_(knockbackDirection)
{
}

void PlayerStateHurt::Enter(Player* player)
{
    direction_.y = 0.0f;
    if (direction_.LengthSq() < 0.0001f)
    {
        // 向きが分からなければ、後ろへ下がる
        direction_ = -player->GetTransform().rotation_.RotateVector({ 0.0f, 0.0f, 1.0f });
        direction_.y = 0.0f;
    }
    direction_ = direction_.Normalize();

    // 殴ってきた方を向いたまま後ろへ押される
    const float angleY = std::atan2(-direction_.x, -direction_.z);
    player->GetTransform().rotation_ = Quaternion::QuaternionFromEuler({ 0.0f, angleY, 0.0f });

    if (AnimationManager::GetInstance()->Get(kHitAnimation))
    {
        player->PlayAnimation(kHitAnimation, false, 1.0f, kBlendTime);
    }
    else
    {
        player->PlayAnimation(kFallbackAnimation, true, 1.0f, kBlendTime);
    }
}

void PlayerStateHurt::Update(Player* player)
{
    elapsedTime_ += TimeManager::GetInstance()->GetDeltaTime();

    // 出だしが一番速く、すぐ止まる
    const float knockbackTime = std::max(player->config.hitKnockbackTime, 0.01f);
    const float progress = std::min(elapsedTime_ / knockbackTime, 1.0f);
    if (progress < 1.0f)
    {
        const float averageSpeed = player->config.hitKnockbackDistance / knockbackTime;
        player->ApplyHorizontalMovement(direction_, averageSpeed * 2.0f * (1.0f - progress));
    }

    player->ApplyGravity();
    player->SnapToGround();

    if (elapsedTime_ < std::max(player->config.hitStunTime, knockbackTime))
    {
        return;
    }

    if (!player->IsGrounded())
    {
        player->GetStateMachine()->ChangeState<PlayerStateJump>();
    }
    else
    {
        player->GetStateMachine()->ChangeState<PlayerStateNormal>();
    }
}
