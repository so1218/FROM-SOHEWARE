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
    void SetTargetToFollow(WorldTransform* target, const Vector3& offset);

    void Play(); // エミッターの再生を開始/リスタート
    void Stop(); // エミッターの再生を停止

    void Destroy();

    Vector3 position_;
    float spawnInterval_;
    float lifetime_;
    float timeSinceLastSpawn_;
    int amount_;
    float duration_;
    bool looping_;
    bool isPlaying_ = false;  // 現在再生中か
    float elapsedTime_ = 0.0f;// 再生開始からの経過時間
    WorldTransform* targetToFollow_ = {};
    Vector3 followOffset_ = { 0.0f, 0.0f, 0.0f };

    std::string name_ = "Emitter";
    std::string& presetName_ = name_;

    EmitterConfig emitterConfig_;
    ParticleConfig particleConfig_;

    bool isDead_ = false;
};




