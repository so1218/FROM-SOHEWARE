#ifdef __cplusplus
#include "MathUtils.h" 
#include "WorldTransform.h"

#define float4x4 Matrix4x4
#define float4 Vector4
#define float3 Vector3
#define float2 Vector2
#define float32_t float

#else

#endif

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

struct FrameData
{
    float4x4 viewProjectionMatrix;

    float3 cameraWorldPosition;
    float padding0;
    float3 cameraRight;
    float padding1;
    float3 cameraUp;
    float padding2;

    float2 iResolution; 
    float2 screenResolution;
    
    float gTime; 
    float nearClip;
    float farClip; 
    float padding3;
};

struct MaterialData
{
    float4x4 uvTransform;
    float4 color;

    int32_t enableLighting;
    int32_t lightMode;
    float shininess; 
    float environmentMapIntensity;
    
    float4 specularColor; 
    
    float diffuseReflection;
    int32_t addShadow;
    float shadowBias; 
    float shadowDensity; 

    int enableRim; 
    float rimPower;
    float rimIntensity; 
    float emissiveIntensity;
    
    float3 rimColor;
    int32_t rimUseLightDir;

    int32_t isArtWave;
    int32_t isArtSound;
    int32_t isArtQuad;
    int32_t isArtKikagaku;
    int32_t isArtFrag;
    int32_t isArtGrid;
    float2 padding2; 
};

struct TrailMaterialData
{
    float2 scrollSpeed;
    float jitterStrength; 
    float jitterFrequency;
    
    float jitterSpeed; 
    float jitterPhase;
    float dissolveThreshold; 
    int isDissolveEnabled;
    
    int jitterMode;
    float emissiveIntensity;
    float2 padding;
    
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
    
    float4x4 viewProj;
    
    int32_t enable;
    float3 padding; 
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    int32_t enable;
    float padding; 
};

struct SpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    int32_t enable;
    float padding;
};

struct AreaLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 right;
    float range;
    float3 up;
    float decay;
    int32_t enable;
    float3 padding; 
};

struct ParticleInstanceData
{
    float4x4 worldMatrix;
    float4 color;
    uint32_t textureIndex;
    float rotationZ;
    int32_t isBillboard; 
    float intensity;
};

struct BrightExtractSettings
{
    float threshold; 
    float intensity; 
    float2 _padding;
};

struct DoFSettingsData
{
    float focusDistance;
    float focusRange;
    float bokehRadius;
    float _padding;
};

struct CombineSettings
{
    float bloomIntensity;
    float focusDistance;
    float focusRange; 
    int enableDoF;
    
    float3 fogColor;
    float fogStart;
    
    float fogEnd; 
    int enableFog;
    float2 padding2;
};

struct OutlineData
{
    float4 color;
    float width;
    float padding[3];
};
