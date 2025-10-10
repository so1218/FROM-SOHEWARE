#pragma once
#include "Easing.h"
#include "WorldTransform.h"

#include <memory>
#include <string>

class ParticleSystem;

enum class ParticleType
{
    None,

    Key,
    HitEffect,

    Count
};

inline const char* ParticleTypeToString(ParticleType type) {
    switch (type) {
    case ParticleType::None: return "None";
    case ParticleType::Key: return "Key";
    case ParticleType::HitEffect: return "HitEffect";
    default: return "Unknown";
    }
}

inline ParticleType StringToParticleType(const std::string& str) {
    if (str == "None") return ParticleType::None;
    if (str == "Key") return ParticleType::Key;
    if (str == "HitEffect") return ParticleType::HitEffect;
    return ParticleType::None; // fallback
}

struct ParticleConfig
{
    ParticleType type;
    float gravity;
    float drag;
    float decayRate;
    float maxLifetime;
    uint32_t textureIndex;
    float radius;
    Vector4 baseColor;

    float speed;
    Vector3 emitterRange;
    unsigned int startColor;
    unsigned int endColor;
    Vector3 startScale;
    Vector3 endScale;
    EasingType fadeOutEasing;
    EasingType scaleEasing;
    int fadeOutDurationFrames;
    Easing fadeOutEase;
    Easing toCenterEase;
    Easing scaleEase;
    Vector3 initialPosition;

    ParticleConfig()
    {
        startColor = 0xffffffff;
        endColor = 0xffffff00;
        speed = 1.0f;
        startScale = { 0.0f,0.0f,0.0f };
        endScale = { 1.0f,1.0f,1.0f };
        emitterRange = { 20, 20, 20 };
        initialPosition = { 0.0f,0.0f,0.0f };
    }
};


struct ParticleState
{
    std::unique_ptr<WorldTransform> transform;
    Vector4 color;
    ParticleType type;
    uint32_t textureHandle;
    float temperature;
    float lifetime;
    float density;
    Vector3 velocity;
    float age = 0.0f;
    bool hasLifetime;
    float rotationSpeed;

    bool hasExisted;
    bool isEmit;
    int frameCount;
    int appearInterval;
    Vector3 emitterRange;
    Vector3 prePos = {};
    Vector3 acceleration;
    Vector3 startPos;
    float speed;
    int spawnFrame_ = 0;
    float thetaVel = 0;
    int amount = 0;
    Vector3 startScale = { 0.0f,0.0f,0.0f };
    Vector3 endScale = { 1.0f,1.0f,1.0f };
    bool isExist;
    unsigned int startColor;
    unsigned int endColor;
    Easing fadeOutEase;
    Easing toCenterEase;
    Easing scaleEase;
    float theta;
    Vector3 initialPosition; // 生成時のエミッターの座標

    ParticleState()
    {

        // デフォルト値で初期化
        isExist = false;
        hasExisted = false;
        frameCount = 0;
        appearInterval = 4;
        startColor = 0xffffffff;
        endColor = 0xffffff00;
        speed = 1.0f;
        emitterRange = { 20, 20, 20 };
        isEmit = false;
        initialPosition = { 0.0f,0.0f,0.0f };
    }
};

// エミッターの基本的な設定を保持する構造体
struct EmitterConfig {
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    float spawnInterval = 0.1f; // 発生間隔 (秒)
    float lifetime = 5.0f;      // パーティクルの生存時間
    int amount = 1;             // 一度に発生させる量
    // 必要に応じて、範囲(range)や初期速度(initial velocity)なども追加できます
};

// パーティクルタイプごとの定義をまとめる構造体
struct ParticleDefinition {
    ParticleConfig particleConfig;
    EmitterConfig emitterConfig;
};

class IParticleBehavior 
{
public:
    virtual ~IParticleBehavior() = default;
    virtual void Initialize(ParticleState& particle, const ParticleSystem& system) = 0;
    virtual void Update(ParticleState& particle) = 0;
};