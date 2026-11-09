#pragma once
#include "Easing.h"
#include "WorldTransform.h"
#include "MathUtils.h"
#include "BlendMode.h"
#include "TextureManager.h"
#include "Structures.h"

namespace FS
{

class AnimationModel;

struct ShapeModule
{
    enum class Type { Point, Box, Sphere };

    const ModelData* sourceModelData = nullptr;

    bool enabled = true;     
    Type type = Type::Point;  

    // Sphere設定
    Vector3 radius = { 10.0f, 10.0f, 10.0f }; 
    bool emitFromEdge = false;              

    // Box設定
    Vector3 boxSize = { 20.0f, 20.0f, 20.0f };

    // 初期位置オフセットを形状に基づいて計算
    Vector3 GetInitialPositionOffset(const ModelData* overrideModelData = nullptr, const AnimationModel* overrideAnimModel = nullptr) const;
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
            // 中心軸がY軸とほぼ平行な場合は、別のベクトルを使って外積を計算
            if (abs(d_norm.y) > 0.999f) {
                up = { 1.0f, 0.0f, 0.0f };
            }
            Vector3 u = Math::CrossProduct(d_norm, up).Normalize();
            Vector3 v = Math::CrossProduct(d_norm, u);

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
    float drag = 0.0f;                 

    bool affectedByWind = false;         
    float windInfluence = 1.0f;           
};

struct RotationOverLifetimeModule
{
    bool enabled = false;             
    bool isBillboard = true;           

    // 2D (Billboard) 用の速度範囲
    float minAngularVelocity2D = 0.0f;
    float maxAngularVelocity2D = 5.0f;

    // 3D 用の速度範囲
    Vector3 minAngularVelocity3D = { 0.0f, 0.0f, 0.0f };
    Vector3 maxAngularVelocity3D = { 0.0f, 0.0f, 0.0f };

    // 初期角度
    Vector3 minStartRotation = { 0.0f, 0.0f, 0.0f };
    Vector3 maxStartRotation = { 0.0f, 0.0f, 360.0f };

    bool randomStartRotation = true;    
};

struct ColorOverLifetimeModule
{
    enum class Mode : int
    {
        Single = 0,            
        RandomBetweenTwo = 1   
    };

    bool enabled = true;         
    Mode mode = Mode::Single;   

    unsigned int startColor = 0xffffffff;  
    unsigned int endColor = 0xffffff00;    
    unsigned int startColor2 = 0xffffffff; 
    unsigned int endColor2 = 0xffffff00;  
    EasingType easingType = EasingType::EaseLinear;

    Vector4 Evaluate(float t) const
    {
        float eased_t = Easing::Evaluate(this->easingType, t);  
        Vector4 startVec = Math::Uint32ToColorVector(startColor);
        Vector4 endVec = Math::Uint32ToColorVector(endColor);
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
            float sin_wave = sinf(t * frequency * 2.0f * Math::PI);
            float eased_t = sin_wave * 0.5f + 0.5f;
            return Math::Lerp(startScale, endScale, eased_t);
        }
        else
        {
            // 通常のイージング
            float eased_t = Easing::Evaluate(this->easingType, t);

            // 加工された時間を使って補間
            return Math::Lerp(startScale, endScale, eased_t);
        }
    }
};

struct TextureSheetAnimationModule
{
    bool enabled = false;
    std::string textureName = "white1x1";
    uint32_t textureHandle = 0; 

    // テクスチャ名からハンドルを取得・更新
    void RefreshHandle()
    {
        textureHandle = TextureManager::GetInstance().Get(textureName);
    }
};

struct NoiseModule
{
    enum class NoiseType
    {
        Simplex, 
        Curl    
    };

    bool enabled = false;
    NoiseType type = NoiseType::Simplex;

    float strength = 1.0f;     
    float frequency = 0.05f;   
    float scrollSpeed = 0.5f; 

    bool separateAxes = true;  
};

struct AttractionModule
{
    bool enabled = false;
    Vector3 target = { 0.0f, 0.0f, 0.0f }; 
    Vector3 offset = { 0.0f, 0.0f, 0.0f };
    float strength = 1.0f;               
};

enum class TrailAlignment
{
    View,      
    Transform  
};

enum class JitterMode
{
    Smooth = 0, 
    Random = 1  
};

struct TrailModule
{
    bool enabled = false;
    float lifetime = 0.5f;
    float width = 1.0f;
    float minVertexDistance = 0.1f;
    std::string textureName = "white1x1";

    Vector4 startColor = { 1, 1, 1, 1 };
    Vector4 endColor = { 1, 1, 1, 0 };

    TrailAlignment alignment = TrailAlignment::View;

    float headWidthScale = 1.0f;
    float tailWidthScale = 1.0f;

    // ジッターの設定
    JitterMode jitterMode = JitterMode::Smooth;
    float jitterStrength = 0.0f;
    float jitterFrequency = 10.0f;
    float jitterSpeed = 0.0f;
    float jitterPhase = 0.0f;

    float emissiveIntensity = 1.0f;
};

struct TrailPoint
{
    Vector3 position;
    Quaternion rotation;
    float time; 
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
    TrailModule trail;
    AttractionModule attraction;
    NoiseModule noise;

    ParticleConfig()
    {
        initialPosition = { 0.0f,0.0f,0.0f };
        baseColor = { 1.0f,1.0f,1.0f,1.0f };
    }

    // ロード時や初期化時にハンドルを更新
    void RefreshResources()
    {
        textureSheet.RefreshHandle();
    }
};

struct ParticleState
{
    WorldTransform transform;
    Vector4 color;
    Vector3 velocity;
    Vector3 currentAngularVelocity = { 0.0f, 0.0f, 0.0f };
    // 角速度で回すので、回転はオイラー角で持つ
    Vector3 rotation = { 0.0f, 0.0f, 0.0f };
    Vector3 initialPosition;

    float lifetime = 0.0f;
    float age = 0.0f;
    float trailSeed = 0.0f;

    const WorldTransform* attractionTarget = nullptr;

    const ParticleConfig* config = nullptr;

    // トレイル用の履歴バッファ
    std::deque<TrailPoint> trailHistory;

    ParticleState() = default;
};

// エミッター設定
struct EmitterConfig
{
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    float spawnInterval = 0.1f;
    float lifetime = 4.0f;     
    int amount = 1;            
    float duration = -0.1f;     
    bool looping = true;       
    bool playOnAwake = true;    
    Vector3 followOffset = { 0.0f, 0.0f, 0.0f };
};

// パーティクル・エミッター設定のセット
struct ParticleDefinition
{
    ParticleConfig particleConfig;
    EmitterConfig emitterConfig;
};

}