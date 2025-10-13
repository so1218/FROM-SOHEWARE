#pragma once
#include "MathUtils.h"
#include "WorldTransform.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <d3d12.h> 
#include <wrl.h> 
#include <cstdint>

struct VertexData
{
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
};

struct MaterialData
{
    std::string textureFilePath;
    uint32_t textureHandle = 0;
};

struct LineVertex
{
    Vector4 position;
    Vector3 color;
};

struct Material
{
    Matrix4x4 uvTransform;

    Vector4 color;     

    int32_t enableLighting;
    int32_t lightMode;
    int32_t isArtWave;
    int32_t isArtSound;

    int32_t isArtQuad;
    int32_t isArtKikagaku;
    int32_t isArtFrag;
    int32_t isArtGrid;

    Vector2 iResolution;
    float gTime; // グローバル時間
    float shininess;   

    Vector4 specularColor;         
};

struct LineMaterial
{
    Vector4 color;
};

enum class MaterialType
{
    Complex,
    Line,
};

struct MaterialHandle
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    Material* materialData;
    LineMaterial* lineMaterialData;
    MaterialType type;
};

struct Node
{
	WorldTransform transform;
    Matrix4x4 localMatrix;
	std::string name;
    std::vector<Node> children;
};

struct ModelData
{
    std::vector<VertexData>vertices;
    std::vector<uint32_t>indices;
    MaterialData material;
    MaterialHandle materialHandle;
    uint32_t textureHandle;
	Node rootNode;
};

struct VertexKey 
{
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;

    bool operator==(const VertexKey& other) const
    {
        return position.x == other.position.x &&
            position.y == other.position.y &&
            position.z == other.position.z &&
            position.w == other.position.w &&
            texcoord.x == other.texcoord.x &&
            texcoord.y == other.texcoord.y &&
            normal.x == other.normal.x &&
            normal.y == other.normal.y &&
            normal.z == other.normal.z;
    }
};

// ハッシュ関数の定義
namespace std
{
    template <>
    struct hash<VertexKey>
    {
        size_t operator()(const VertexKey& key) const {
            size_t h = 0;
            h ^= std::hash<float>()(key.position.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>()(key.position.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>()(key.position.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>()(key.position.w) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>()(key.texcoord.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>()(key.texcoord.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>()(key.normal.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>()(key.normal.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>()(key.normal.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };
}

struct TransformationMatrix
{
    Matrix4x4 WVP;
    Matrix4x4 World;
    Matrix4x4 WorldInverseTranspose;
};

struct MaterialSettings
{
    Matrix4x4 uvTransform;

    Vector4 color;

    bool enableLighting;
    int32_t lightMode;
    int32_t isArtWave;
    int32_t isArtSound;

    int32_t isArtQuad;
    int32_t isArtKikagaku;
    int32_t isArtFrag;
    int32_t isArtGrid;

    Vector2 iResolution;
    float gTime; 
    float shininess;

    Vector4 specularColor;
};

struct DirectionalLight
{
    Vector4 color;// ライトの色
    Vector3 direction;// ライトの向き
    float intensity;// 輝度
    int enable;
    float padding[3];     // 明示的に 16 バイト追加（これで合計 48 bytes）
};

struct PointLight
{
    Vector4 color;// ライトの色
    Vector3 position;// ライトの位置
    float intensity;// 輝度
    float radius;
    float decay;
    int enable;
    float padding;
};

struct SpotLight
{
    Vector4 color;
    Vector3 position;
    float intensity;
    Vector3 direction;
    float distance;
    float decay;
    float cosAngle;
    int enable;
    float padding;
};

struct ParticleInstanceData
{
    Matrix4x4 worldMatrix;
    Vector4 color;
    uint32_t textureIndex;
    float rotationZ;
    float padding[2];

};

struct CameraBuffer
{
    Matrix4x4 viewProjectionMatrix;
    Vector3 cameraRight; // X軸方向
	float padding0; // パディング
    Vector3 cameraUp; // Y軸方向

};

struct AABB
{
    Vector3 min;
    Vector3 max;
};

struct CameraForGPU
{
    Vector3 worldPosition;
    float padding0;
};

struct BloomSettingsCPU {
    float brightnessThreshold;
    float padding[3];
   
};

struct CombineSettingsCPU {
    float bloomIntensity;
    float padding[3]; 
};

struct BlurSettingsCPU {
    Vector2 texelSize; 
    float padding[2];            
};

struct PostEffectData
{
    int mode; 

    float brightnessValue; 
    float posterizationLevels; 
    float pixelationSize; 

    Vector2 screenResolution;
    float grayscaleColorAmount;
    float sepiaColorAmount;

    float invertColorAmount;
    float tintMulColorAmount;
    float tintAddColorAmount;
    float tintScreenColorAmount;

    Vector3 tintColor; 
    float totalTime;

    float contrastValue; 
    float saturationValue;
    float hueShiftAmount;           
    float vignetteAmount;

    float vignetteRadius;
    float vignetteSoftness;
    Vector2 vignetteEllipseScale;

    int channelSwapMode;
    float noiseAmount;
    float noiseSpeed;
    float noiseScale;

    float celShadingLevels;
    float normalOutlineThreshold;
    float normalOutlineThickness;
    float _paddingNormlOutline1;


    Vector3 normalOutlineColor;
    float _paddingNormlOutline2;

    float chromaOffset;
    float waveAmplitude; 
    float waveFrequency; 
    int waveDirection;

    float waveSpeed;
    float fisheyeDistortion;
    Vector2 _paddingFisheye;

    float flashFrequency;
    float flashIntensity;
    float scanlineIntensity;
    float scanlineFrequency;

    int scanlineDirection;
    Vector3 scanlineColor;

    float scanlineScrollSpeed;
    Vector3 _paddingscanline;

    float blockNoiseAmount;
    float blockNoiseSize;
    float blockNoiseSpeed;
    float solarizeThreshold;

    float multiPosterizeLevels; 
    float rgbSplitOffset;
    float filmGrainIntensity;
    float _padding1;

    float glitchBlockHeight;
    float glitchAmount;
    float glitchNoiseIntensity;
    float edgeThreshold;

    float heatDistortionStrength; 
    float heatSpeed; 
    float heatNoiseScale;
    float _padding2;

    Vector3 vignetteColor;
    float _padding3;

    Vector3 shadowColor;
    float _paddingSplit;

    Vector3 highlightColor;
    float splitToneStrength;

    float turbulentStrength;
    float turbulentFrequency;
    float turbulentSpeed;
    float _paddingTurbulence;

    float roughEdgeThreshold;
    float roughEdgeRoughness;
    float roughEdgeNoiseScale;
    float roughEdgeSpeed;

    Vector3 roughEdgeColor;
    float _paddingRoughEdge;

    float spiralBaseAmplitude;
    float spiralFrequency;
    float spiralDistanceFalloff;
    float spiralNoiseAmount;

    float spiralNoiseSpeed;
    float spiralNoiseScale;
    float spiralRotationSpeed;
    float spiralSpeed;

    float radialWaveSpeed;
    float radialWaveAmplitude;
    float radialWaveFrequency;
    float _paddingRadialWave;

    float glowOutlineThreshold;
    float glowOutlineThickness;
    float glowOutlineIntensity;
    float _paddingGlow0;

    Vector3 glowOutlineColor;
    float _paddingGlow1;

    uint32_t modeFlags[2];
    Vector2 _paddingGlow2;

    int fbmOctaves;
    float fbmGain;
    float fbmLacunarity;
    float fbmSharpness; 

    float fbmNoiseIntensity; 
    Vector3 fbmNoiseColor;

    Vector3 flareColor;
    float flareIntensity;

    float flareFalloff;
    float flareGhostDistance;
    float flareGhostIntensity;
    float flareStreakCount;

    float flareStreakSpeed;
    float flareStreakSharpness;
    float flareStreakIntensity;
    float paddingFlare;

    Vector2 ballRadiusValue;
    Vector2 ballPosition;

    float ballNoiseAmount;
    Vector3 ballColorAdjustment;

    float ballTimeSpeed;
    float dotBlinkSize;
    float dotBlinkSpeed;
    float _paddingdotBlink;
};

struct BrightExtractSettings
{
    float threshold = 0.7f;
    float intensity = 1.0f;
    Vector2 padding = { 0.0f, 0.0f }; 
};

struct BlurSettings
{
    Vector2 texelSize = { 1.0f / 1280.0f, 1.0f / 720.0f };
    float blurStrength = 1.0f;
    float padding; 
};

struct CombineSetting
{
    float brightnessThreshold = 0.75f;
    int effectMode; // 0: Halo, 1: Neon
    Vector2 padding;
};

struct DepthExtractSettingsVS
{
    float nearPlane;
    float farPlane;
    Vector2 padding; 
    Matrix4x4 invViewProjection;
};

struct DepthExtractSettingsPS
{
    float nearPlane;
    float farPlane;
    Vector2 padding;
};