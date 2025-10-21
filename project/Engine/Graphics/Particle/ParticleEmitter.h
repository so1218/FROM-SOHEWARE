#pragma once
#include "Vector3.h"
#include "WorldTransform.h"
#include "ParticleSystem.h"

class ParticleEmitter
{
public:
    void Initialize(const EmitterConfig& config);

    void Update(ParticleSystem& particleSystem);

    // Emitterの位置設定
    void SetPosition(const Vector3& position) { position_ = position; }
    void SetTargetToFollow(WorldTransform* target, const Vector3& offset);

    void Play(); // エミッターの再生を開始/リスタート
    void Stop(); // エミッターの再生を停止

    Vector3 position_;
    float spawnInterval_;
    float lifetime_;
    float timeSinceLastSpawn_;
    int amount_;
    float duration_;
    bool looping_;
    bool isPlaying_ = false;  // 現在再生中か
    float elapsedTime_ = 0.0f;// 再生開始からの経過時間
    WorldTransform* targetToFollow_ = nullptr;
    Vector3 followOffset_ = {};

	std::string presetName_ = "Emitter";

    EmitterConfig emitterConfig_;
    ParticleConfig particleConfig_;
};


