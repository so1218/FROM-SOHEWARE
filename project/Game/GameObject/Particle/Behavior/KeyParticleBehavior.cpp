#include "KeyParticleBehavior.h"
#include "TextureHandle.h"

void KeyParticleBehavior::Initialize(ParticleState& particle, const ParticleConfig& config)
{
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
    particle.fadeOutEase.SetEasing(EasingType::EaseOutCirc);
    particle.scaleEase.SetEasing(EasingType::EaseLinear);
    particle.scaleEase.frameCount_ = config.scaleEase.frameCount_;

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
                particle.isExist = true;
                particle.fadeOutEase.isEase_ = true;
                particle.hasExisted = true;

                if (particle.config.rotation.enabled && particle.config.rotation.randomStartRotation)
                {
                    particle.transform->rotation_.z = RandomFloat(0.0f, 360.0f);
                }

                particle.transform->translation_ = particle.initialPosition + particle.config.shape.GetInitialPositionOffset();
            }
            particle.frameCount = 0;
        }
    }
    particle.frameCount++;
    if (particle.isExist)
    {
        // 位置を更新する前に、物理的な力を速度に適用する
        if (particle.config.physics.enabled)
        {
            // 重力を適用（Y軸の速度を減少させる）
            particle.velocity.y -= particle.config.physics.gravity;

            // 空気抵抗を適用（速度全体を少しずつ減速させる）
            particle.velocity = particle.velocity * (1.0f - particle.config.physics.drag);
        }

        // （物理演算によって変化した）速度を位置に反映
        particle.transform->translation_ += particle.velocity;

        // 回転の更新
         // モジュールの角速度に基づいて回転させる
        if (particle.config.rotation.enabled)
        {
            particle.transform->rotation_.z += particle.config.rotation.angularVelocity;
        }
        particle.transform->rotationQuaternion_ = Quaternion::QuaternionFromEuler(particle.transform->rotation_);

        if (!particle.fadeOutEase.isEase_)
        {
            particle.isExist = false;
        }
        unsigned int currentColor = (unsigned int)ColorVectorToUint32(particle.color);
        if (particle.fadeOutEase.isEase_)
        {
            particle.fadeOutEase.CountEaseLinear(particle.startColor, particle.endColor, currentColor);
        }
        particle.color = Uint32ToColorVector(currentColor);
        particle.scaleEase.OnceReverseEaseLinear(particle.startScale, particle.endScale, particle.transform->scale_);
    }
}