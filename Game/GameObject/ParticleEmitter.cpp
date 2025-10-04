#include "ParticleEmitter.h"

void ParticleEmitter::Initialize(ParticleType type, const Vector3& position, float spawnInterval, float lifetime, int amount)
{
    type_ = type;
    position_ = position;
    spawnInterval_ = spawnInterval;
    lifetime_ = lifetime;
    amount_ = amount;
    timeSinceLastSpawn_ = 0.0f;
}

void ParticleEmitter::Update(float deltaTime, ParticleSystem& particleSystem)
{
    timeSinceLastSpawn_ += deltaTime;

    // 定期的にパーティクルを生成
    while (timeSinceLastSpawn_ >= spawnInterval_)
    {

        WorldTransform worldTransform = {
            { 1.0f, 1.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f },
            position_,
        };
        particleSystem.SpawnParticle(worldTransform, type_, lifetime_, amount_);
        timeSinceLastSpawn_ -= spawnInterval_;
    }
}