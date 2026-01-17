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

#define NONE                0
#define GRAYSCALE           (1 << 0)
#define SEPIA               (1 << 1)
#define PIXELATION          (1 << 2)
#define COLOR_TINT          (1 << 3)
#define VIGNETTE            (1 << 4)
#define SCREEN_NOISE        (1 << 5)
#define CHROM_ABERRATION    (1 << 6)
#define SCREEN_WAVE         (1 << 7)
#define FISHEYE             (1 << 8)
#define SCANLINE            (1 << 9)
#define BLOCK_NOISE         (1 << 10)
#define RGB_SPLIT           (1 << 11)
#define FILM_GRAIN          (1 << 12)
#define GLITCH              (1 << 13)
#define HEAT_HAZE           (1 << 14)
#define WATER_REFRACTION    (1 << 15)
#define DISSOLVE            (1 << 16)
#define RADIAL_BLUR         (1 << 17)

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
    
    float shadowSoftness;
    int32_t isArtGrid;
    float2 padding2;

    int enableRim;
    float rimPower;
    float rimIntensity;
    float emissiveIntensity;
    
    float3 rimColor;
    int32_t rimUseLightDir;

    int32_t enableDissolve;
    float3 edgeColor;

    float dissolveThreshold;
    float edgeWidth;
    float edgeIntensity;
    int32_t enableNormalMap;
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

struct DoFSettings
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
    float godRayIntensity;
    float _padding1;
};

struct GodRaySettings
{
    float2 lightPosScreen;
    float density;
    float decay;
    
    float weight;
    float exposure;
    float threshold;
    int numSamples;
    
    float3 lightColor;
    float sunRadius;
};

struct OutlineData
{
    float4 color;
    float width;
    float padding[3];
};
