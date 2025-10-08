#pragma once
#include "Vector3.h"
#include "WorldTransform.h"
#include "Particle.h"

class ParticleEmitter
{
public:
    void Initialize(ParticleType type, const Vector3& position, float spawnInterval, float lifetime, int amount);

    void Update(ParticleSystem& particleSystem);

    // Emitterの位置設定
    void SetPosition(const Vector3& position) { position_ = position; }

    // Emitterのタイプ設定
    void SetType(ParticleType type) { type_ = type; }


    ParticleType type_;
    Vector3 position_;
    float spawnInterval_;
    float lifetime_;
    float timeSinceLastSpawn_;
    int amount_;

    std::string name_ = "Emitter";
};


