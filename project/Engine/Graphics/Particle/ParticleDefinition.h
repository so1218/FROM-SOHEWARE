#pragma once
#include "Easing.h"
#include "WorldTransform.h"
#include "MathUtils.h"

#include <memory>
#include <string>
#include <deque>

struct ShapeModule
{
    enum class Type { Point, Box, Sphere };

    bool enabled = true;      // モジュールが有効かどうか
    Type type = Type::Point;  // 形状の種類

    // Sphere設定
    Vector3 radius = { 10.0f, 10.0f, 10.0f }; // 各軸方向の半径
    bool emitFromEdge = false;                // 縁からのみ生成するか

    // Box設定
    Vector3 boxSize = { 20.0f, 20.0f, 20.0f }; // 各軸方向のサイズ

    // 初期位置オフセットを形状に基づいて計算
    Vector3 GetInitialPositionOffset() const
    {
        switch (type)
        {
        case Type::Point:
            return { 0.0f, 0.0f, 0.0f }; // 原点のみ

        case Type::Box:
            // ボックス内部のランダムな位置
            return {
                Math::RandomFloat(-boxSize.x / 2.0f, boxSize.x / 2.0f),
                Math::RandomFloat(-boxSize.y / 2.0f, boxSize.y / 2.0f),
                Math::RandomFloat(-boxSize.z / 2.0f, boxSize.z / 2.0f)
            };

        case Type::Sphere:
        {
            // 単位球上のランダムな点を生成
            float phi = Math::RandomFloat(0.0f, 2.0f * 3.14159f);
            float cosTheta = Math::RandomFloat(-1.0f, 1.0f);
            float theta = acosf(cosTheta);

            Vector3 unitSpherePoint = {
                sinf(theta) * cosf(phi),
                sinf(theta) * sinf(phi),
                cosf(theta)
            };

            // 半径0の軸は0に固定
            if (radius.x == 0.0f) unitSpherePoint.x = 0.0f;
            if (radius.y == 0.0f) unitSpherePoint.y = 0.0f;
            if (radius.z == 0.0f) unitSpherePoint.z = 0.0f;

            // 単位ベクトル化（縁上に配置）
            unitSpherePoint = unitSpherePoint.Normalize();

            // 各軸に沿って拡大（楕円体化）
            Vector3 ellipsoidPoint = {
                unitSpherePoint.x * radius.x,
                unitSpherePoint.y * radius.y,
                unitSpherePoint.z * radius.z
            };

            // emitFromEdgeがfalseなら中心寄りに縮小
            if (!emitFromEdge)
                ellipsoidPoint = ellipsoidPoint * cbrtf(Math::RandomFloat(0.0f, 1.0f));

            return ellipsoidPoint;
        }
        }

        return { 0.0f, 0.0f, 0.0f }; // デフォルト
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
            Vector3 u = Math::CrossProduct(d_norm, up).Normalize();
            Vector3 v = Math::CrossProduct(d_norm, u); // uとd_normが直交かつ正規化済みなので、vも正規化される

            // 円錐状に広がるためのランダムな角度を2つ生成
            float phi = Math::RandomFloat(0.0f, 2.0f * Math::PI);
            // theta: 中心軸からの広がり角度 (0° ～ angleRange/2)
            float maxAngleRad = (angleRange / 2.0f) * (Math::PI / 180.0f);
            float cosTheta = Math::RandomFloat(cosf(maxAngleRad), 1.0f);
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
    // どのモードで色を決定するか
    enum class Mode : int
    { 
        Single = 0,
        RandomBetweenTwo = 1
    };

    bool enabled = true;
    Mode mode = Mode::Single;

    unsigned int startColor = 0xffffffff;
    unsigned int endColor = 0xffffff00;
    // グラデーション2 (RandomBetweenTwo用)
    unsigned int startColor2 = 0xffffffff; // (例: 2つ目の開始色)
    unsigned int endColor2 = 0xffffff00;   // (例: 2つ目の終了色)

    EasingType easingType = EasingType::EaseLinear;

    Vector4 Evaluate(float t) const
    {
        // 1. Easingオブジェクトで時間tを加工
        float eased_t = Easing::Evaluate(this->easingType, t);

        // 2. 色を計算しやすいVector4に変換
        Vector4 startVec = Math::Uint32ToColorVector(startColor);
        Vector4 endVec = Math::Uint32ToColorVector(endColor);

        // 3. 加工された時間を使って補間
        return Math::Lerp(startVec, endVec, eased_t);
    }
};

struct SizeOverLifetimeModule 
{
    bool enabled = true;
    Vector3 startScale = { 1.0f, 1.0f, 1.0f };
    Vector3 endScale = { 0.0f, 0.0f, 0.0f };
    EasingType easingType = EasingType::EaseLinear;

    bool oscillate = false;
    float frequency = 1.0f;

    Vector3 Evaluate(float t) const
    {
        if (oscillate)
        {
            float sin_wave = sinf(t * frequency * 2.0f * 3.14159f);
            float eased_t = sin_wave * 0.5f + 0.5f;
            return Math::Lerp(startScale, endScale, eased_t);
        }
        else
        {
            // 通常のイージング
            float eased_t = Easing::Evaluate(this->easingType, t); 

            // 加工された時間を使って補間する
            return Math::Lerp(startScale, endScale, eased_t);
        }
    }
};

struct TextureSheetAnimationModule
{
    bool enabled = false;
    uint32_t textureHandle = 0; // スプライトシート全体のテクスチャハンドル
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

struct AttractionModule
{
    bool enabled = false;
    Vector3 target = { 0.0f, 0.0f, 0.0f }; // 引き寄せられる目標地点（中心）
    float strength = 1.0f;              // 引き寄せられる強さ（加速度）
};

enum class TrailTextureMode
{
    Stretch, // 全体を0〜1で伸ばす
    Tile     // 距離に応じて繰り返す
};

enum class TrailAlignment
{
    View,      // ビルボード
    Transform  // パーティクルの回転に沿う
};

enum class JitterMode
{
    Wave,   // 0: 滑らか
    Step,   // 2: 規則的 (四角・階段) ★追加
    Random, // 1: ランダム (稲妻)
};

struct TrailModule
{
    bool enabled = false;
    float lifetime = 0.5f;
    float width = 1.0f;
    float minVertexDistance = 0.1f;
    uint32_t textureHandle = 0;

    Vector4 startColor = { 1, 1, 1, 1 };
    Vector4 endColor = { 1, 1, 1, 0 };

    TrailTextureMode textureMode = TrailTextureMode::Stretch;
    Vector2 tiling = { 1, 1 };       // UV の繰り返し数
    Vector2 scrollSpeed = { 0, 0 };  // UV スクロール速度

    TrailAlignment alignment = TrailAlignment::View;

    float headWidthScale = 1.0f; // 先端の太さ
    float tailWidthScale = 1.0f; // 尻尾の太さ

    // ジッター（揺れ）の設定
    float jitterStrength = 0.0f;
    float jitterFrequency = 10.0f;
    float jitterSpeed = 0.0f;
    float jitterPhase = 0.0f;

    // ディゾルブの設定
    uint32_t dissolveTextureHandle = 0;
    float dissolveSpeed = 2.0f;

    JitterMode jitterMode = JitterMode::Wave;
};

struct TrailPoint
{
    Vector3 position;
    Quaternion rotationQuaternion;
    float time; // 生成された時刻
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

    // トレイル用の履歴バッファ
    std::deque<TrailPoint> trailHistory;

    ParticleConfig config;

    ParticleState()
    {
        // デフォルト値で初期化
        initialPosition = { 0.0f,0.0f,0.0f };
    }
};

// エミッター設定
struct EmitterConfig
{
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    float spawnInterval = 0.1f; // パーティクルの生成間隔
    float lifetime = 4.0f;      // パーティクル寿命
    int amount = 1;             // 1回の生成数
    float duration = -0.1f;     // エミッター稼働時間（負なら無限）
    bool looping = true;        // duration 終了後にループするか
    bool playOnAwake = true;    // 生成時に自動再生するか
};

// パーティクル・エミッター設定のセット
struct ParticleDefinition
{
    ParticleConfig particleConfig;
    EmitterConfig emitterConfig;
};