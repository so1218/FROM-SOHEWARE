#include "pch.h"
#include "BurnStatus.h"
#include "RendererManager.h"

using namespace FS;

void BurnConfig::Bind(PropertyBinder& binder, const std::string& prefix, const BurnConfig& defaults)
{
    binder.Bind(prefix + "FireThreshold", &fireThreshold, defaults.fireThreshold, 0.01f, 0.0f, 1.0f);
    binder.Bind(prefix + "DamagePerSecond", &damagePerSecond, defaults.damagePerSecond, 0.5f, 0.0f, 1000.0f);
    binder.Bind(prefix + "TickInterval", &tickInterval, defaults.tickInterval, 0.01f, 0.05f, 10.0f);
    binder.Bind(prefix + "AfterburnTime", &afterburnTime, defaults.afterburnTime, 0.05f, 0.0f, 30.0f);
}

void BurnConfig::Draw(PropertyBinder& binder, const std::string& prefix)
{
    binder.Draw(prefix + "FireThreshold", "燃え始める炎の強さ");
    binder.Draw(prefix + "DamagePerSecond", "1 秒あたりのダメージ (一番強い炎の上)");
    binder.Draw(prefix + "TickInterval", "ダメージを入れる間隔 (秒)");
    binder.Draw(prefix + "AfterburnTime", "炎から出たあとも燃え続ける秒数");
}

int BurnStatus::Update(const RendererManager* rendererManager, const Vector3& footPosition, const BurnConfig& config, float deltaTime)
{
    const float fire = rendererManager ? rendererManager->SampleGrassFire(footPosition.x, footPosition.z) : 0.0f;
    const bool wasBurning = IsBurning();
    const float tickInterval = std::max(config.tickInterval, 0.05f);

    isInFire_ = fire > config.fireThreshold;
    afterburnTime_ = config.afterburnTime;
    if (isInFire_)
    {
        // 燃え始めは間隔の半分だけ待ち、炎の縁をかすめただけでは痛くないようにする
        if (!wasBurning)
        {
            tickTimer_ = tickInterval * 0.5f;
        }
        burnTimer_ = std::max(config.afterburnTime, 0.01f);
        fireStrength_ = std::min(fire, 1.0f);
    }
    else
    {
        burnTimer_ = std::max(burnTimer_ - deltaTime, 0.0f);
    }

    if (!IsBurning())
    {
        tickTimer_ = 0.0f;
        damageRemainder_ = 0.0f;
        return 0;
    }

    tickTimer_ -= deltaTime;
    if (tickTimer_ > 0.0f)
    {
        return 0;
    }
    tickTimer_ += tickInterval;

    damageRemainder_ += config.damagePerSecond * tickInterval * GetStrength();
    const int damage = static_cast<int>(damageRemainder_);
    damageRemainder_ -= static_cast<float>(damage);
    return damage;
}

void BurnStatus::Extinguish()
{
    isInFire_ = false;
    burnTimer_ = 0.0f;
    fireStrength_ = 0.0f;
    tickTimer_ = 0.0f;
    damageRemainder_ = 0.0f;
}

float BurnStatus::GetStrength() const
{
    if (!IsBurning())
    {
        return 0.0f;
    }
    if (isInFire_)
    {
        return fireStrength_;
    }
    return (afterburnTime_ > 0.0f) ? fireStrength_ * (burnTimer_ / afterburnTime_) : 0.0f;
}
