#ifdef __cplusplus
#include "MathUtils.h" 
#include "WorldTransform.h"

#define float4x4 Matrix4x4
#define float4 Vector4
#define float3 Vector3
#define float2 Vector2
#define int32_t int32_t
#define uint32_t uint32_t
#define float32_t float

#else

#endif

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

struct Material
{
    float4x4 uvTransform;

    float4 color;

    int32_t enableLighting;
    int32_t lightMode;
    int32_t isArtWave;
    int32_t isArtSound;

    int32_t isArtQuad;
    int32_t isArtKikagaku;
    int32_t isArtFrag;
    int32_t isArtGrid;

    float2 iResolution;
    float gTime;
    float shininess;

    float4 specularColor;

    float environmentMapIntensity;
    float3 padding;
};

struct CameraForGPU 
{
    float3 worldPosition;
    float32_t padding0;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
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
    float padding; 
};

struct CameraBuffer
{
    float4x4 viewProjectionMatrix;
    float3 cameraRight;
    float padding0;
    float3 cameraUp;
    float padding1;
};