#include "ParticleEmitter.h"
#include "TimeManager.h"

void ParticleEmitter::Initialize(const EmitterConfig& config)
{
    // EmitterConfigからすべての設定をコピー
    position_ = config.position;
    spawnInterval_ = config.spawnInterval;
    lifetime_ = config.lifetime;
    amount_ = config.amount;
    duration_ = config.duration;
    looping_ = config.looping;
    followOffset_ = config.followOffset;

    timeSinceLastSpawn_ = 0.0f;
    isPlaying_ = false;

    // playOnAwakeがtrueなら、自動的に再生
    if (config.playOnAwake)
    {
        Play();
    }
}

void ParticleEmitter::SetTargetToFollow(WorldTransform* target)
{
    targetToFollow_ = target;
}

void ParticleEmitter::Update(ParticleSystem& particleSystem)
{
    // 再生中でなければ何もしない
    if (!isPlaying_)
    {
        return;
    }

    if (targetToFollow_)
    {
        position_ = targetToFollow_->translation_ + followOffset_;
    }

    // 経過時間を更新
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    elapsedTime_ += deltaTime;

    // Durationを超えたかチェック
    if (duration_ >= 0.0f && elapsedTime_ >= duration_)
    {
        if (looping_)
        {
            // ループ再生なら時間をリセット
            elapsedTime_ -= duration_;
        }
        else
        {
            // ループしないなら停止して終了
            Stop();
            return;
        }
    }

    timeSinceLastSpawn_ += deltaTime;

    // 定期的にパーティクルを生成
    while (timeSinceLastSpawn_ >= spawnInterval_)
    {
        // amount_の数だけループしてパーティクルを生成
        for (int i = 0; i < amount_; ++i)
        {
            WorldTransform worldTransform = 
            {
                { 1.0f, 1.0f, 1.0f },
                { 0.0f, 0.0f, 0.0f },
                position_,
            };
            // 1つのパーティクルを生成
            particleSystem.SpawnParticle(worldTransform, presetName_, lifetime_, attractionTarget_);
        }

        timeSinceLastSpawn_ -= spawnInterval_;
    }
}

void ParticleEmitter::Play()
{
    isPlaying_ = true;
    elapsedTime_ = 0.0f;
    timeSinceLastSpawn_ = 0.0f; // 放出タイミングもリセット
}

void ParticleEmitter::Stop()
{
    isPlaying_ = false;
}

void ParticleEmitter::Destroy()
{
    isDead_ = true;
    Stop();
}