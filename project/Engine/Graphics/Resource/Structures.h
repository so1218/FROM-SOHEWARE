#pragma once
#include "MathUtils.h"
#include "WorldTransform.h"
#include "ShaderConstants.hlsli"

#include <unordered_map>
#include <string>
#include <vector>
#include <d3d12.h> 
#include <wrl.h> 
#include <cstdint>
#include <map>
#include <span>

struct VertexData
{
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
    Vector3 smoothNormal;
};

// Trail専用の頂点構造体
struct VertexDataTrail
{
    Vector4 pos;  
    Vector2 tex;  
    Vector4 color;
};

struct TextureData
{
    std::string textureFilePath;
    uint32_t textureHandle = 0;
};

struct LineVertex
{
    Vector4 position;
    Vector3 color;
};

struct MaterialHandle
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    MaterialData* materialData;
};

struct VertexWeightData
{
    float weight;
    uint32_t vertexIndex;
};

struct JointWeightData
{
    Matrix4x4 inverseBindPoseMatrix;
    std::vector<VertexWeightData> vertexWeights;
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
    TextureData textureData;
    MaterialHandle materialHandle;
	Node rootNode;
	std::map<std::string, JointWeightData> skinClusterData;
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

struct MaterialSettings
{
    Matrix4x4 uvTransform = Matrix4x4::MakeIdentity();

    Vector4 color = Vector4(1, 1, 1, 1);

    bool enableLighting = false;
    int32_t lightMode = 1;
    float shininess = 50.0f;
    float environmentMapIntensity = 0.0f;
    float4 specularColor = Vector4(1, 1, 1, 1);

    float diffuseReflection = 4.0f;
    bool addShadow = true;
    float shadowBias = 0.0005f;
    float shadowDensity = 0.7f;

    bool enableRim = false;
    float rimPower = 3.0f;    
    float rimIntensity = 1.0f;
    Vector3 rimColor = { 1.0f, 1.0f, 1.0f }; 

    bool rimUseLightDir = false;

    float emissiveIntensity = 1.0f;

    int32_t isArtWave = false;
    int32_t isArtSound = false;
    int32_t isArtQuad = false;
    int32_t isArtKikagaku = false;
    int32_t isArtFrag = false;
    int32_t isArtGrid = false;
    float2 padding2;
};

struct AABB
{
    Vector3 min;
    Vector3 max;
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
};

struct BlurSettings
{
    Vector2 texelSize = { 1.0f / 1280.0f, 1.0f / 720.0f };
    float blurStrength = 1.0f;
    float padding; 
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
