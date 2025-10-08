#include "KeyParticleBehavior.h"
#include "TextureHandle.h"

void KeyParticleBehavior::Initialize(ParticleState& particle, const ParticleSystem& particleSystem)
{
    auto& config = particleSystem.GetConfig(ParticleType::Key);

    particle.appearInterval = 4;
    particle.emitterRange = config.emitterRange;
    particle.fadeOutEase->interval_ = config.fadeOutEase->interval_;
    particle.color = { 127.0f,255.0f,0.0f,255.0f };
    particle.startColor = config.startColor;
    particle.endColor = config.endColor;
    particle.textureHandle = config.textureIndex;
    particle.isExist = true;
    particle.hasExisted = false;
    particle.frameCount = 0;
    particle.isEmit = false;
    particle.speed = config.speed;
    particle.fadeOutEase->SetEasing(EasingType::EaseOutCirc);
    particle.scaleEase->SetEasing(EasingType::EaseLinear);
    //particle.fadeOutEase->SetEaseDurationFrames(250);
    particle.scaleEase->interval_ = config.scaleEase->interval_;

#ifdef _DEBUG

    particle.appearInterval = 4;
    particle.color = { 127.0f,255.0f,0.0f,255.0f };
    particle.isExist = true;
    particle.hasExisted = false;
    particle.frameCount = 0;
    particle.isEmit = false;
    particle.fadeOutEase->SetEasing(EasingType::EaseOutCirc);
    particle.scaleEase->SetEasing(EasingType::EaseLinear);
    //particle.fadeOutEase->SetEaseDurationFrames(250);
#endif

}

void KeyParticleBehavior::Update(ParticleState& particle)
{
    if (particle.frameCount >= particle.amount * particle.appearInterval)
    {
        particle.hasExisted = false;
        particle.frameCount = 0;
    }
    // フレームごとに新しいパーティクルを生成
    if (!particle.hasExisted)
    {
        if (particle.frameCount >= particle.spawnFrame_)
        {
            if (!particle.isExist)
            {
                // ランダムで角度を設定
                particle.theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * float(PI);

                // 半径をランダムに生成 (0～emitterRange_ の範囲)
                float radius = static_cast<float>(RandomFloat(0.05f, static_cast<float>(particle.emitterRange.x)));

                // 極座標 -> 直交座標
                particle.transform->translation_.x = static_cast<float>(particle.transform->translation_.x) + radius * cos(particle.theta);
                particle.transform->translation_.y = static_cast<float>(particle.transform->translation_.y + 0.5f) + radius * sin(particle.theta);
                particle.velocity.x = particle.speed * cosf(particle.theta);
                particle.velocity.y = particle.speed * sinf(particle.theta);
                particle.isExist = true;
                particle.fadeOutEase->isEase_ = true;
                particle.hasExisted = true;
               

                if (rand() % 2 == 0)
                {
                    particle.thetaVel = float(rand() % 2 + 0.01f);
                }
                else
                {
                    particle.thetaVel = -float(rand() % 2 + 0.01f);
                }

            }
            particle.frameCount = 0;
        }
    }
    particle.frameCount++;
    if (particle.isExist)
    {
        particle.transform->translation_.x += particle.velocity.x;
        particle.transform->translation_.y += particle.velocity.y;

        particle.transform->rotation_.z += particle.thetaVel;
        particle.transform->rotationQuaternion_ = Quaternion::QuaternionFromEuler(particle.transform->rotation_);

        if (!particle.fadeOutEase->isEase_)
        {
            particle.isExist = false;
        }
        unsigned int currentColor = (unsigned int)ColorVectorToUint32(particle.color);
        if (particle.fadeOutEase->isEase_)
        {
            particle.fadeOutEase->CountEaseLinear(particle.startColor, particle.endColor, currentColor);
        }
        particle.color = Uint32ToColorVector(currentColor);
        particle.scaleEase->OnceReverseEaseLinear({ 0.0f,0.0f, 0.0f }, { 2.0f,2.0f,0.1f }, particle.transform->scale_);
    }
}