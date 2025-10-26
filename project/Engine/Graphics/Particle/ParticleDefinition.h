#pragma once
#include "Easing.h"
#include "WorldTransform.h"
#include "MathUtils.h"

#include <memory>
#include <string>

struct ShapeModule
{
    enum class Type { Point, Box, Sphere };

    bool enabled = true;
    Type type = Type::Point;

    // Sphere設定
    Vector3 radius = { 10.0f, 10.0f, 10.0f };
    bool emitFromEdge = false; // 縁からのみ生成するか

    // Box設定
    Vector3 boxSize = { 20.0f, 20.0f, 20.0f };

    // このモジュールに基づいて初期位置のオフセットを計算する関数
    Vector3 GetInitialPositionOffset() const
    {
        switch (type)
        {
        case Type::Point:
            return { 0.0f, 0.0f, 0.0f };

        case Type::Box:
            // 内部（体積）から生成するロジック
            return {
                RandomFloat(-boxSize.x / 2.0f, boxSize.x / 2.0f),
                RandomFloat(-boxSize.y / 2.0f, boxSize.y / 2.0f),
                RandomFloat(-boxSize.z / 2.0f, boxSize.z / 2.0f)
            };
        case Type::Sphere:
        {
            // 半径1の単位球上のランダムな点を生成する
            float phi = RandomFloat(0.0f, 2.0f * 3.14159f);
            float cosTheta = RandomFloat(-1.0f, 1.0f);
            float theta = acosf(cosTheta);

            Vector3 unitSpherePoint = {
                sinf(theta) * cosf(phi),
                sinf(theta) * sinf(phi),
                cosf(theta)
            };

            // もし特定の軸の半径が0なら、その軸方向の単位球座標を強制的に0にする
            if (radius.x == 0.0f) { unitSpherePoint.x = 0.0f; }
            if (radius.y == 0.0f) { unitSpherePoint.y = 0.0f; }
            if (radius.z == 0.0f) { unitSpherePoint.z = 0.0f; }

            // 正規化して、点が必ず縁に来るようにする
            unitSpherePoint = unitSpherePoint.Normalize();

            // 各軸の半径を使って、単位球/円/線上の点を引き伸ばす
            Vector3 ellipsoidPoint = {
                unitSpherePoint.x * radius.x,
                unitSpherePoint.y * radius.y,
                unitSpherePoint.z * radius.z
            };

            // emitFromEdgeがfalseの場合、中心に向かってランダムに縮小する
            if (!emitFromEdge)
            {
                ellipsoidPoint = ellipsoidPoint * cbrtf(RandomFloat(0.0f, 1.0f));
            }

            return ellipsoidPoint;
        }
        }
        return { 0.0f, 0.0f, 0.0f };
    }
};

struct VelocityModule
{
    bool enabled = true;
    float speed = 4.0f;
    bool randomDirection = true;
    float angleRange = 60.0f; 
    Vector3 direction = { 0.0f, 1.0f, 0.0f };

    Vector3 GetInitialVelocity() const
    {
        if (randomDirection)
        {
            // 中心となる方向ベクトルを正規化
            Vector3 d_norm = direction.Normalize();

            // d_normと直交する2つのベクトル(u, v)を生成し、局所的な座標系を作る
            Vector3 up = { 0.0f, 1.0f, 0.0f };
            // 中心軸がY軸とほぼ平行な場合は、別のベクトルを使って外積を計算する
            if (abs(d_norm.y) > 0.999f) {
                up = { 1.0f, 0.0f, 0.0f };
            }
            Vector3 u = CrossProduct(d_norm, up).Normalize();
            Vector3 v = CrossProduct(d_norm, u); // uとd_normが直交かつ正規化済みなので、vも正規化される

            // 円錐状に広がるためのランダムな角度を2つ生成
            float phi = RandomFloat(0.0f, 2.0f * PI);
            // theta: 中心軸からの広がり角度 (0° ～ angleRange/2)
            float maxAngleRad = (angleRange / 2.0f) * (PI / 180.0f);
            float cosTheta = RandomFloat(cosf(maxAngleRad), 1.0f);
            float theta = acosf(cosTheta);

            // 局所座標系でランダムな方向ベクトルを計算
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
    Vector3 gravity = { 0.0f, -9.8f, 0.0f };
    float drag = 0.0f; // 空気抵抗の割合 (0.01 = 1%減速)
};

struct RotationOverLifetimeModule
{
    bool enabled = false;
    bool isBillboard = true;
     // isBillboardがtrueの場合
    float angularVelocity2D = 5.0f; // 1秒あたりの回転角度（度数法）

    // isBillboardがfalseの場合
    Vector3 angularVelocity3D = { 0.0f, 0.0f, 0.0f };
    // 生成時の向きをオイラー角(度数法)で指定
    Vector3 orientation3D = { 0.0f, 0.0f, 0.0f };

    bool randomStartRotation = true;
};

struct ColorOverLifetimeModule
{
    bool enabled = true;
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

struct SizeOverLifetimeModule 
{
    bool enabled = true;
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

    SizeOverLifetimeModule() 
    {
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

struct NoiseModule
{
    bool enabled = false;
    float strength = 1.0f;   // 揺らぎの強さ
    float frequency = 1.0f;  // 揺らぎの細かさ（周波数）
    float scrollSpeed = 1.0f; // ノイズが時間と共に流れる速度
    bool separateAxes = false; // X, Y, Z軸で別々の設定を使うか
};

struct VortexModule 
{
    bool enabled = false;
    Vector3 center = { 0.f, 0.f, 0.f }; // 渦の中心
    float rotationSpeed = 90.0f;    // 1秒あたりの回転速度
    float orbitalSpeed = 10.0f;     // 中心へ向かう/離れる速度（負の値で離れる）
};

struct TrailModule
{
    bool enabled = false;
    float lifetime = 0.5f; // 軌跡が消えるまでの時間
    // 色や太さを軌跡の始点から終点にかけて変えるためのグラデーション設定など
};

struct AttractionModule
{
    bool enabled = false;
    Vector3 target = { 0.0f, 0.0f, 0.0f }; // 引き寄せられる目標地点（中心）
    float strength = 1.0f;              // 引き寄せられる強さ（加速度）
};

struct ParticleConfig
{
    uint32_t textureIndex;
    Vector3 initialPosition;
    Vector4 baseColor;

    VelocityModule velocity;
    SizeOverLifetimeModule sizeOverLifetime;
    ColorOverLifetimeModule colorOverLifetime;
    PhysicsModule physics; 
    RotationOverLifetimeModule rotation; 
    ShapeModule shape;
    TextureSheetAnimationModule textureSheet;
    VortexModule vortex;
    TrailModule trail;
    AttractionModule attraction;

    ParticleConfig()
    {
        initialPosition = { 0.0f,0.0f,0.0f };
        baseColor = { 1.0f,1.0f,1.0f,1.0f };
    }
};

struct ParticleState
{
    std::unique_ptr<WorldTransform> transform;
    Vector4 color;
    uint32_t textureHandle;
    float lifetime;
    Vector3 velocity;
    float age = 0.0f;

    Vector3 initialPosition; // 生成時のエミッターの座標
    std::string presetName;
    Vector4 uvRect = { 0.0f, 0.0f, 1.0f, 1.0f };

    ParticleConfig config;

    ParticleState()
    {
        // デフォルト値で初期化
        initialPosition = { 0.0f,0.0f,0.0f };
    }
};


// エミッターの基本的な設定を保持する構造体
struct EmitterConfig
{
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    float spawnInterval = 0.1f; // 発生間隔 (秒)
    float lifetime = 4.0f;      // パーティクルの生存時間
    int amount = 1;             // 一度に発生させる量
    float duration = -0.1f; // エミッターが動作し続ける時間（秒）。負の値で無限。
    bool looping = true;   // durationが経過した後、ループするか
    bool playOnAwake = true;// 生成時に自動で再生を開始するか
};

// パーティクルタイプごとの定義をまとめる構造体
struct ParticleDefinition 
{
    ParticleConfig particleConfig;
    EmitterConfig emitterConfig;
};