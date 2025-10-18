#include "ParticleEmitter.h"
#include "TimeManager.h"

void ParticleEmitter::Initialize(const Vector3& position, float spawnInterval, float lifetime, int amount)
{
    position_ = position;
    spawnInterval_ = spawnInterval;
    lifetime_ = lifetime;
    amount_ = amount;
    timeSinceLastSpawn_ = 0.0f;
}

void ParticleEmitter::Update(ParticleSystem& particleSystem)
{
    timeSinceLastSpawn_ += TimeManager::GetInstance()->GetDeltaTime();

    // 定期的にパーティクルを生成
    while (timeSinceLastSpawn_ >= spawnInterval_)
    {
        // amount_の数だけループしてパーティクルを生成
        for (int i = 0; i < amount_; ++i)
        {
            WorldTransform worldTransform = {
                { 1.0f, 1.0f, 1.0f },
                { 0.0f, 0.0f, 0.0f },
                position_,
            };
            // 1つのパーティクルを生成
            particleSystem.SpawnParticle(worldTransform, presetName_, lifetime_);
        }

        timeSinceLastSpawn_ -= spawnInterval_;
    }
}