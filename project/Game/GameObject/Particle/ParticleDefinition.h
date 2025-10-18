#pragma once
#include "Easing.h"
#include "WorldTransform.h"
#include "MathUtils.h"

#include <memory>
#include <string>

enum class ParticleType
{
    None,

    Key,
    HitEffect,

    Count
};

inline const char* ParticleTypeToString(ParticleType type)
{
    switch (type)
    {
    case ParticleType::None: return "None";
    case ParticleType::Key: return "Key";
    case ParticleType::HitEffect: return "HitEffect";
    default: return "Unknown";
    }
}

inline ParticleType StringToParticleType(const std::string& str) 
{
    if (str == "None") return ParticleType::None;
    if (str == "Key") return ParticleType::Key;
    if (str == "HitEffect") return ParticleType::HitEffect;
    return ParticleType::None; // fallback
}

struct ShapeModule
{
    enum class Type { Point, Box, Sphere, Circle };

    bool enabled = true;
    Type type = Type::Circle;

    // Circle / Sphere 共通設定
    float radius = 10.0f;
    bool emitFromEdge = false; // 縁からのみ生成するか

    // Box 設定
    Vector3 boxSize = { 20.0f, 20.0f, 20.0f };

    // このモジュールに基づいて初期位置のオフセットを計算する関数
    Vector3 GetInitialPositionOffset() const
    {
        switch (type)
        {
        case Type::Point:
            return { 0.0f, 0.0f, 0.0f };

        case Type::Box:
            return {
                RandomFloat(-boxSize.x / 2.0f, boxSize.x / 2.0f),
                RandomFloat(-boxSize.y / 2.0f, boxSize.y / 2.0f),
                RandomFloat(-boxSize.z / 2.0f, boxSize.z / 2.0f)
            };

        case Type::Circle:
        {
            float r = emitFromEdge ? radius : radius * sqrtf(RandomFloat(0.0f, 1.0f));
            float theta = RandomFloat(0.0f, 2.0f * 3.14159f);
            return { cosf(theta) * r, sinf(theta) * r, 0.0f };
        }

        case Type::Sphere:
        {
            // 球体状に均一な点を生成
            float phi = RandomFloat(0.0f, 2.0f * 3.14159f);
            float cosTheta = RandomFloat(-1.0f, 1.0f);
            float theta = acosf(cosTheta);
            float r = emitFromEdge ? radius : radius * cbrtf(RandomFloat(0.0f, 1.0f));

            return {
                r * sinf(theta) * cosf(phi),
                r * sinf(theta) * sinf(phi),
                r * cosf(theta)
            };
        }
        }
        return { 0.0f, 0.0f, 0.0f };
    }
};

struct VelocityModule
{
    bool enabled = false;
    float speed = 1.0f;
    bool randomDirection = false;
    float angleRange = 0.0f; 
    Vector3 direction = { 1.0f, 0.0f, 0.0f };

    Vector3 GetInitialVelocity() const
    {
        if (randomDirection)
        {
            // 1. 中心となる方向ベクトルを正規化
            Vector3 d_norm = direction.Normalize();

            // 2. d_normと直交する2つのベクトル(u, v)を生成し、局所的な座標系を作る
            Vector3 up = { 0.0f, 1.0f, 0.0f };
            // 中心軸がY軸とほぼ平行な場合は、別のベクトルを使って外積を計算する
            if (abs(d_norm.y) > 0.999f) {
                up = { 1.0f, 0.0f, 0.0f };
            }
            Vector3 u = CrossProduct(d_norm, up).Normalize();
            Vector3 v = CrossProduct(d_norm, u); // uとd_normが直交かつ正規化済みなので、vも正規化される

            // 3. 円錐状に広がるためのランダムな角度を2つ生成
            // phi: 中心軸周りの回転角度 (0° ～ 360°)
            float phi = RandomFloat(0.0f, 2.0f * PI);
            // theta: 中心軸からの広がり角度 (0° ～ angleRange/2)
            // cosを使って分布を均一にする
            float maxAngleRad = (angleRange / 2.0f) * (PI / 180.0f);
            float cosTheta = RandomFloat(cosf(maxAngleRad), 1.0f);
            float theta = acosf(cosTheta);

            // 4. 局所座標系でランダムな方向ベクトルを計算
            Vector3 randomDir =
                (u * cosf(phi) * sinf(theta)) +
                (v * sinf(phi) * sinf(theta)) +
                (d_norm * cosf(theta));

            return randomDir.Normalize() * speed;
        }
        else 
        {
            return direction.Normalize() * speed;
        }
    }
};

struct PhysicsModule
{
    bool enabled = false;
    float gravity = 0.0f;
    float drag = 0.0f; // 空気抵抗の割合 (0.01 = 1%減速)
};

struct RotationOverLifetimeModule
{
    bool enabled = false;
    bool randomStartRotation = true;// 開始時の角度をランダムにするか
    float angularVelocity = 5.0f; // 1フレームあたりの回転角度（度数法）
};

struct ColorOverLifetimeModule
{
    bool enabled = false;
    unsigned int startColor = 0xffffffff;
    unsigned int endColor = 0xffffff00;
    Easing easing;
    ColorOverLifetimeModule()
    {
        easing.SetEasing(EasingType::EaseLinear);
        easing.frameCount_ = 60;
    }

    Vector4 Evaluate(float t) const
    {
        // 1. Easingオブジェクトで時間tを加工
        float eased_t = easing.Evaluate(t);

        // 2. 色を計算しやすいVector4に変換
        Vector4 startVec = Uint32ToColorVector(startColor);
        Vector4 endVec = Uint32ToColorVector(endColor);

        // 3. 加工された時間を使って補間
        return Lerp(startVec, endVec, eased_t);
    }
};

struct SizeOverLifetimeModule {
    bool enabled = false;
    Vector3 startScale = { 1.0f, 1.0f, 1.0f };
    Vector3 endScale = { 0.0f, 0.0f, 0.0f };
    Easing easing;

    bool oscillate = false;
    float frequency = 1.0f;

    Vector3 Evaluate(float t) const
    {
        if (oscillate) {
            // 振動する場合
            float sin_wave = sinf(t * frequency * 2.0f * 3.14159f);
            float eased_t = sin_wave * 0.5f + 0.5f;
            return Lerp(startScale, endScale, eased_t);
        }
        else {
            // 通常のイージング
            // 1. Easingオブジェクトで時間tを加工する
            float eased_t = easing.Evaluate(t);
            // 2. 加工された時間を使って補間する
            return Lerp(startScale, endScale, eased_t);
        }
    }

    SizeOverLifetimeModule() {
        // デフォルトのイージングタイプを設定
        easing.SetEasing(EasingType::EaseLinear);
    }
};

struct TextureSheetAnimationModule
{
    bool enabled = false;
    uint32_t textureHandle = 0; // スプライトシート全体のテクスチャハンドル

    int tilesX = 1; // 横方向の分割数
    int tilesY = 1; // 縦方向の分割数

    float framesPerSecond = 10.0f; // 1秒あたりのフレーム数
    bool looping = true;
};

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
    bool isInfinite = false;

    VelocityModule velocity;
    SizeOverLifetimeModule sizeOverLifetime;
    ColorOverLifetimeModule colorOverLifetime;
    PhysicsModule physics; 
    RotationOverLifetimeModule rotation; 
    ShapeModule shape;
    TextureSheetAnimationModule textureSheet;

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
    std::string presetName;
    int spawnedCount = 0;        // 生成済みの数
    bool isSpawning = false;// 現在生成中かどうかのフラグ
    bool isInfinite = false;
    Vector4 uvRect = { 0.0f, 0.0f, 1.0f, 1.0f };

    ParticleConfig config;

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

};

// パーティクルタイプごとの定義をまとめる構造体
struct ParticleDefinition {
    ParticleConfig particleConfig;
    EmitterConfig emitterConfig;
};