#pragma once
#include "Easing.h"
#include "WorldTransform.h"
#include "MathUtils.h"
#include "BlendMode.h"

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
    bool enabled = false;               // モジュールの有効/無効
    Vector3 gravity = { 0.0f, -9.8f, 0.0f }; // 重力加速度
    float drag = 0.0f;                  // 空気抵抗 (0.01 = 1% 減速)
};

struct RotationOverLifetimeModule
{
    bool enabled = false;               // モジュールの有効/無効
    bool isBillboard = true;            // ビルボード回転か3D回転か

    // 2D (Billboard) 用の速度範囲
    float minAngularVelocity2D = 0.0f;
    float maxAngularVelocity2D = 5.0f;

    // 3D 用の速度範囲
    Vector3 minAngularVelocity3D = { 0.0f, 0.0f, 0.0f };
    Vector3 maxAngularVelocity3D = { 0.0f, 0.0f, 0.0f };

    // 初期角度
    Vector3 minStartRotation = { 0.0f, 0.0f, 0.0f };
    Vector3 maxStartRotation = { 0.0f, 0.0f, 360.0f };

    bool randomStartRotation = true;    // 初期回転をランダムにするか
};

struct ColorOverLifetimeModule
{
    enum class Mode : int
    {
        Single = 0,             // 単一グラデーション
        RandomBetweenTwo = 1    // 2種類の色からランダムに選択
    };

    bool enabled = true;         // モジュールの有効/無効
    Mode mode = Mode::Single;    // 色決定モード

    unsigned int startColor = 0xffffffff;  // 開始色
    unsigned int endColor = 0xffffff00;    // 終了色
    unsigned int startColor2 = 0xffffffff; // 2つ目の開始色 (RandomBetweenTwo用)
    unsigned int endColor2 = 0xffffff00;   // 2つ目の終了色 (RandomBetweenTwo用)

    EasingType easingType = EasingType::EaseLinear; // 補間のイージングタイプ

    Vector4 Evaluate(float t) const
    {
        float eased_t = Easing::Evaluate(this->easingType, t);  // 時間をイージング
        Vector4 startVec = Math::Uint32ToColorVector(startColor);
        Vector4 endVec = Math::Uint32ToColorVector(endColor);
        return Math::Lerp(startVec, endVec, eased_t);           // 色を補間
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
    bool enabled = false;           // アニメーションの有効/無効
    uint32_t textureHandle = 0;     // スプライトシート全体のテクスチャハンドル
};

struct CollisionModule
{
    bool enabled = false;

    enum class Type
    {
        Plane, // 無限平面 (床など)
        World, // ワールド内の特定の球や箱
    };
    Type type = Type::Plane;

    float bounce = 0.5f;     // 跳ね返り (0=吸着, 1=完全反射)
    float friction = 0.0f;   // 摩擦 (床を滑る抵抗)
    float dampen = 0.0f;     // 衝突後の速度減衰 (0=なし, 1=停止)
    float lifeLoss = 0.0f;   // 衝突時に失う寿命 (1=即死)
    float minKillSpeed = 0.0f;// この速度以下で衝突したら消滅

    // 形状設定
    struct PlaneData
    {
        Vector3 point = { 0.0f, 0.0f, 0.0f }; // 平面上の点
        Vector3 normal = { 0.0f, 1.0f, 0.0f };// 法線
    } plane;

    // 形状設定
    struct WorldObject 
    {
        enum class Shape { Sphere, Box };
        Shape shape = Shape::Sphere;
        Vector3 center = { 0.0f, 0.0f, 0.0f };
        Vector3 scale = { 2.0f, 2.0f, 2.0f }; 
    } worldObj;

};

struct NoiseModule
{
    bool enabled = false;
    float strength = 1.0f;    // 揺らぎの強さ
    float frequency = 1.0f;   // 揺らぎの細かさ
    float scrollSpeed = 1.0f; // 時間に応じてノイズが流れる速度
    bool separateAxes = false; // X/Y/Z軸で別々のノイズを使うか
};

struct VortexModule
{
    bool enabled = false;
    Vector3 center = { 0.0f, 0.0f, 0.0f }; // 渦の基準点
    Vector3 axis = { 0.0f, 1.0f, 0.0f };   // 回転軸
    float orbitalSpeed = 2.0f; // 周回スピード（接線方向）
    float radialSpeed = 0.0f;  // 中心へ向かうスピード（負の値で外へ広がる）
};

struct AttractionModule
{
    bool enabled = false;
    Vector3 target = { 0.0f, 0.0f, 0.0f }; // 静的な目標位置（ターゲット未設定時）
    Vector3 offset = { 0.0f, 0.0f, 0.0f };// 動的ターゲットに対するオフセット
    float strength = 1.0f;                 // 引力の強さ（加速度）
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
    Step,   // 1: 規則的 (四角・階段)
    Random, // 2: ランダム (稲妻)
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
    float intensity = 1.0f;
    BlendMode blendMode = BlendMode::kBlendModeAdd;

    VelocityModule velocity;
    SizeOverLifetimeModule sizeOverLifetime;
    ColorOverLifetimeModule colorOverLifetime;
    PhysicsModule physics; 
    RotationOverLifetimeModule rotation; 
    ShapeModule shape;
    TextureSheetAnimationModule textureSheet;
    CollisionModule collision;
    VortexModule vortex;
    TrailModule trail;
    AttractionModule attraction;
    NoiseModule noise;

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
    const WorldTransform* attractionTarget = nullptr;
    Vector3 currentAngularVelocity = { 0.0f, 0.0f, 0.0f };

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
    Vector3 followOffset = { 0.0f, 0.0f, 0.0f };// 追従時のオフセット座標
};

// パーティクル・エミッター設定のセット
struct ParticleDefinition
{
    ParticleConfig particleConfig;
    EmitterConfig emitterConfig;
};