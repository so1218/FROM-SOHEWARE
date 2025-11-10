#include "ShakeEffect.h"
#include "MathUtils.h"
#include "TimeManager.h"

void ShakeEffect::Start(float duration, float intensity)
{
    duration_ = duration;
    intensity_ = intensity;
    timer_ = 0.0f;
    isActive_ = true;
}

void ShakeEffect::Update()
{
    if (!isActive_)
    {
        wasActive_ = false;
        return;
    }

    timer_ += TimeManager::GetInstance()->GetDeltaTime();

    if (timer_ >= duration_) 
    {
        isActive_ = false;
    }

    // Updateの最後で、前回の状態を記録
    wasActive_ = true;
}

Vector3 ShakeEffect::GetOffset() const
{
    if (!isActive_)
    {
        return { 0.0f, 0.0f, 0.0f };
    }

    // 揺れの進行度（0.0〜1.0）
    float progress = timer_ / duration_;
    float attenuation = 1.0f - progress; // 時間とともに減衰

    // X, Y, Z にランダム値を加える（減衰付き）
    return 
    {
        Math::RandomFloat(-1.0f, 1.0f) * intensity_ * attenuation,
        Math::RandomFloat(-1.0f, 1.0f) * intensity_ * attenuation,
        Math::RandomFloat(-1.0f, 1.0f) * intensity_ * attenuation
    };
}
