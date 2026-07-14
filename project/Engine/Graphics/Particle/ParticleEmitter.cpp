#include "pch.h"
#include "ParticleEmitter.h"
#include "TimeManager.h"

namespace FE
{

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
        Vector3 finalOffset = followOffset_;
        finalOffset.x *= offsetScale_.x;
        finalOffset.y *= offsetScale_.y;
        finalOffset.z *= offsetScale_.z;

        Matrix4x4 targetMatrix = Matrix4x4::MakeAffine(
            targetToFollow_->scale_,
            targetToFollow_->rotationQuaternion_,
            { 0.0f, 0.0f, 0.0f } // 回転とスケールだけ適用
        );

        Vector3 rotatedOffset = targetMatrix.TransformVector(finalOffset);

        Vector3 targetPos = targetToFollow_->translation_ + followOffset_;

        if (followX_) position_.x = targetPos.x;
        if (followY_) position_.y = targetPos.y;
        if (followZ_) position_.z = targetPos.z;

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
            // エミッター自身のTransformを作る際、ターゲットの回転とスケールを引き継ぐ
            WorldTransform spawnTransform;
            spawnTransform.translation_ = position_;
            const ModelData* currentModelData = nullptr;

            if (targetToFollow_)
            {
                spawnTransform.scale_ = targetToFollow_->scale_;
                spawnTransform.rotationQuaternion_ = targetToFollow_->rotationQuaternion_;

                // ターゲットモデルがあればModelDataを取得
                if (targetModel_)
                {
                    currentModelData = targetModel_->GetModelData();
                }
            }
            else
            {
                spawnTransform.scale_ = { 1.0f, 1.0f, 1.0f };
            }

            // 1つのパーティクルを生成
            particleSystem.SpawnParticle(spawnTransform, presetName_, lifetime_, attractionTarget_, vortexTarget_,
                currentModelData, targetAnimModel_);
        }

        timeSinceLastSpawn_ -= spawnInterval_;
    }
}

void ParticleEmitter::Play()
{
    if (!isPlaying_)
    {
        elapsedTime_ = 0.0f;
        timeSinceLastSpawn_ = 0.0f; // 放出タイミングリセット
        isPlaying_ = true;
    }
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

void ParticleEmitter::SetFollowAxes(bool x, bool y, bool z)
{
    followX_ = x;
    followY_ = y;
    followZ_ = z;
}

}