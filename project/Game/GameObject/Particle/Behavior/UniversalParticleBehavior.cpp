#include "UniversalParticleBehavior.h"
#include "TimeManager.h"

void UniversalParticleBehavior::Initialize(ParticleState& particle, const ParticleConfig& config)
{
    particle.emitterRange = config.emitterRange;
    particle.fadeOutEase.frameCount_ = config.fadeOutEase.frameCount_;
    particle.color = { 127.0f,255.0f,0.0f,255.0f };
    particle.startColor = config.startColor;
    particle.endColor = config.endColor;
    particle.startScale = config.startScale;
    particle.endScale = config.endScale;
    particle.textureHandle = config.textureIndex;
    particle.isExist = true;
    particle.hasExisted = false;
    particle.frameCount = 0;
    particle.isEmit = false;
    particle.speed = config.speed;
    particle.fadeOutEase.SetEasing(EasingType::EaseOutCirc);
    particle.scaleEase.SetEasing(EasingType::EaseLinear);
    particle.scaleEase.frameCount_ = config.scaleEase.frameCount_;

}

void UniversalParticleBehavior::Update(ParticleState& particle)
{

  //  // 速度・移動
  //  if (particle.config.velocity.enabled) {
  //      particle.transform->translation_ += particle.velocity;
  //  }

  //  // 色変化
  // /* if (particle.config.colorOverLifetime.enabled) {
  //      particle.color = particle.config.colorOverLifetime.Evaluate();
  //  }*/

  //  // スケール変化
  ///*  if (particle.config.sizeOverLifetime.enabled) {
  //      particle.transform->scale_ = particle.config.sizeOverLifetime.Evaluate();
  //  }*/

  //  particle.age += TimeManager::GetInstance()->GetDeltaTime();
}