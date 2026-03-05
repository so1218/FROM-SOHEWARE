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
#define COLOR_GRADING_LUT   (1 << 18)

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float4 WorldColor;
};

struct FrameData
{
    float4x4 viewMatrix; 
    float4x4 projectionMatrix;
    
    float4x4 viewProjectionMatrix;
    float4x4 invViewProj;
    float4x4 invProjMatrix;

    float3 cameraWorldPosition;
    float padding0;
    float3 cameraRight;
    float padding1;
    float3 cameraUp;
    float padding2;

    float3 mainLightDirection; 
    float paddingLight0;
    float3 mainLightColor; 
    float paddingLight1;

    float2 iResolution;
    float2 screenResolution;
    
    float gTime;
    float nearClip;
    float farClip;
    float padding3;
    
    float3 lightningFlashColor;
    float lightningFlashIntensity;
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
    float shadowEnvStrength;
    int32_t isArtGrid;
    float alphaTestThreshold;

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
    float padding3;
    
    int32_t enableNormalMap;
    float normalTiling;
    float normalIntensity;
    float padding4;
    
    float roughness;
    float metalness;
    float2 paddingMetalness;
    
    float4 outlineColor;
    
    float outlineWidth;
    int32_t enableOutline;
    float2 paddingOutline;

    int32_t enableRipple;
    float wetness; 
    float rippleScale; 
    float rippleSpeed;
    
    float rippleStrength; 
    float puddleScale; 
    float puddleFalloff; 
    float wetDarkness;
    
    int32_t usePuddle;
    float rippleSize;
    float rippleFrequency;
    float rippleLayerMix;
    
    float3 puddleColor;
    float puddleTint;
    
    int32_t isBubble;
    float wobbleSpeed;
    float wobbleAmplitude;
    float rainbowIntensity;
    
    float fresnelExponent;
    float3 paddingBubble;
    
    float grassWindSpeed; 
    float grassWindAmplitude; 
    float grassNormalBlend;
    float grassTranslucency; 

    float grassRootAO; 
    float grassAlphaCutoff; 
    float interactRadius; 
    float interactStrength; 
    
    float3 playerPos;
    int32_t enableTreeWind; 
    
    float treeWindSpeed;
    float treeWindAmplitude; 
    float treeWindSpatialScale;
    float treeWindHeightScale;
    
    float treeWindVariation;
    float3 paddingTree;
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
    float3 position;
    float rotationZ;
    
    float2 scale;
    uint32_t textureIndex;
    int32_t isBillboard;
    
    float4 color;
    float intensity;
    float3 padding;
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
    float transitionRange;
    float bokehRadius;
    
    float bokehHighlightThreshold;
    float bokehHighlightIntensity;
    float2 padding;
};

struct CombineSettings
{
    float bloomIntensity;
    int enableDoF;
    int enableSSAO;
    int enableFog;
    
    float3 fogColor;
    float heightFogDensity;
    
    float heightFogFalloff;
    float heightFogBaseHeight;
    float distanceFogStart;                              
    float distanceFogEnd;
    
    float fogNoiseSpeed;
    float fogNoiseScale;
    float fogNoiseContrast; 
    float fogNoiseStrength;
    
    float godRayIntensity;
    int enableSSR;
    float ssrIntensity;
    float _padding2; 
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

struct SSAOSettings
{
    float radius; 
    float intensity; 
    float bias; 
    int sampleCount;

    float fadeStart; 
    float fadeEnd; 
    float padding[2];
};

struct BilateralBlurSettings
{
    float2 texelSize;
    float2 direction;
    
    float depthTolerance;
    float normalTolerance;
    float2 padding;
};

struct SSRSettings
{
    float maxDistance;
    float stepSize; 
    int maxSteps; 
    float thickness; 
};

struct GrassInstanceData
{
    float4x4 world;
    float4 color;
};

struct Object3DInstanceData
{
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float4 WorldColor;
};

struct InstanceOffset
{
    int gBaseInstanceIndex;
};