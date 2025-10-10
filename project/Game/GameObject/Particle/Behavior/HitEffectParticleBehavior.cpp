#include "HitEffectParticleBehavior.h"
#include "TextureHandle.h"

void HitEffectParticleBehavior::Initialize(ParticleState& particle, const ParticleSystem& system)
{
    particle.appearInterval = 30; // 30フレームごとに発生
    particle.amount = 7;          // 毎回7つ発生
    particle.emitterRange = { 0.0f, 0.0f, 0.0f }; // 同じ場所
    particle.startColor = 0xff00ffff;
    particle.endColor = 0x0000ffff;
    particle.speed = 0.1f;
    particle.thetaVel = 0.0f;
    particle.fadeOutEase.SetEasing(EasingType::EaseOutCirc);
    particle.scaleEase.SetEasing(EasingType::EaseOutBack);
    particle.scaleEase.interval_ = 0.04f;
    particle.isExist = false;
    particle.hasExisted = false;
    particle.frameCount = 0;
    particle.isEmit = false;
}

void HitEffectParticleBehavior::Update(ParticleState& particle)
{
    // 移動、回転、色の補間、寿命処理など
}
