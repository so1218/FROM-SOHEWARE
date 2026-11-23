#include "pch.h"
#include "PlayerStateDeath.h"
#include "AnimationManager.h"

using namespace FS;

namespace
{
constexpr const char* kDeathAnimation = "humanDeath";
constexpr const char* kFallbackAnimation = "humanCrawlIdle";
constexpr float kBlendTime = 0.15f;
}

void PlayerStateDeath::Enter(Player* player)
{
    if (AnimationManager::GetInstance()->Get(kDeathAnimation))
    {
        player->PlayAnimation(kDeathAnimation, false, 1.0f, kBlendTime);
    }
    else
    {
        player->PlayAnimation(kFallbackAnimation, true, 1.0f, kBlendTime);
    }
}

void PlayerStateDeath::Update(Player* player)
{
    // 空中で倒れても地面まで落ちる
    player->ApplyGravity();
    player->SnapToGround();
}
