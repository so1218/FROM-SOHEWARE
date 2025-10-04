#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "WorldTransform.h"
#include "Easing.h"
#include "Structures.h"

#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <array>
#include <fstream>

class Engine;
class ParticleEmitter;

enum class ParticleType
{
    None,

    Key,

    Count
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
    std::unique_ptr<Easing> fadeOutEase;
    std::unique_ptr<Easing> toCenterEase;
    std::unique_ptr<Easing> scaleEase;
    float theta;

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
        fadeOutEase = std::make_unique<Easing>();
        toCenterEase = std::make_unique<Easing>();
        scaleEase = std::make_unique<Easing>();
        isEmit = false;


    }
};

struct ParticleConfig
{
    ParticleType type;
    float gravity;
    float drag;
    float decayRate;
    float maxLifetime;
    uint32_t textureIndex;
    bool reactsToFire;
    bool emitsLight;
    float particleRadius;
    Vector4 baseColor;
};


class ParticleSystem
{
public:
    void Initialize(Engine* engine);
    void SpawnParticle(WorldTransform& transform, ParticleType type, float lifetime, int amount);
    void Update();
    void AddEmitter(ParticleEmitter* emitter);
    void LoadParticleDefinitionsFromJson(const std::string& filepath);

private:

public:
    Engine* engine_;
    std::vector<ParticleEmitter*> emitters_;  // エミッターのリスト
    std::vector<ParticleState> particles_;
    std::array<ParticleConfig, static_cast<size_t>(ParticleType::Count)> particleConfigs_;

};


