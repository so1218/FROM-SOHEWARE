#ifndef SHADER_CONSTANTS_HLSLI
#define SHADER_CONSTANTS_HLSLI

#ifdef __cplusplus
#include "MathUtils.h" 
#include "WorldTransform.h"

#define float4x4 FS::Matrix4x4
#define float4 FS::Vector4
#define float3 FS::Vector3
#define float2 FS::Vector2
#define float32_t float
#define uint uint32_t

#else

#endif

#define NONE                0
#define PIXELATION          (1 << 0)
#define VIGNETTE            (1 << 1)
#define SCREEN_NOISE        (1 << 2)
#define CHROM_ABERRATION    (1 << 3)
#define RADIAL_BLUR         (1 << 4)

#define MAX_DIRECTIONAL_LIGHTS 2
#define MAX_POINT_LIGHTS 10
#define MAX_SPOT_LIGHTS 4

// シャドウマップ
#define MAX_CASCADE_COUNT 3
#define SHADOW_MAP_RESOLUTION 2048.0f

// Light types
#define SHADING_MODEL_HALFLAMBERT 0
#define SHADING_MODEL_PHONG 1
#define SHADING_MODEL_TOON 2
#define SHADING_MODEL_PBR 3
#define LIGHT_POINT 4
#define LIGHT_SPOT 5

#define MAX_FOG_EFFECTORS 4
#define MAX_FOG_VOLUMES 8

// SpriteInstance の drawMode
#define SPRITE_DRAW_MODE_NORMAL   0
#define SPRITE_DRAW_MODE_SDF_TEXT 1

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float4 WorldColor;
    float4x4 PrevWorld;
};

struct FrameData
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    
    float4x4 viewProjectionMatrix;
    float4x4 invViewProj;
    float4x4 invProjMatrix;

    float3 cameraWorldPosition;
    float prevTime;
    
    float3 cameraRight;
    float nearClip;
    
    float3 cameraUp;
    float farClip;

    float3 mainLightDirection;
    float time;
    
    float3 mainLightColor;
    float mainLightVolumetricScatteringIntensity;

    float2 iResolution;
    float2 screenResolution;
    
    float4x4 lightViewProj;
    
    float4x4 prevViewProj;
    
    uint frameIndex;
    float3 prevCameraWorldPosition;
    
    float deltaTime;
    float3 padding;
    
    float4 frustumPlanes[6];
    
};

struct SurfaceData
{
    float3 albedo; 
    float3 pbrAlbedo; 
    float3 specularColor; 
    float3 normal; 
    float roughness; 
    float metalness; 
    float shininess; 
    float diffuseReflection;
    uint lightMode;
};

struct MaterialData
{
    float4x4 uvTransform;
    float4 color;

    int enableLighting;
    int lightMode;
    float shininess;
    float environmentMapIntensity;
    
    float4 specularColor;
    
    float diffuseReflection;
    int addShadow;
    float shadowBias;
    float shadowDensity;
    
    float shadowSoftness;
    float shadowEnvStrength;
    int isArtGrid;
    float alphaTestThreshold;

    int enableRim;
    float rimPower;
    float rimIntensity;
    float emissiveIntensity;
    
    float3 rimColor;
    int rimUseLightDir;

    int enableDissolve;
    float3 edgeColor;

    float dissolveThreshold;
    float edgeWidth;
    float edgeIntensity;
    int enableNormalMap;
    
    float normalTiling;
    float normalIntensity;
    float roughness;
    float metalness;
    
    float4 outlineColor;
    
    float outlineWidth;
    int enableOutline;
    float shadowNormalBias;
    float padding;
};

struct SpriteInstance
{
    float2 position; 
    float2 size; 

    float2 pivot;
    float rotation; 
    uint textureIndex; 

    float4 color; 

    float4 uvRect;

    float4 uvMatrix;
    
    float2 uvTranslate;
    uint dissolveTextureIndex;
    float dissolveThreshold;

    float3 edgeColor;
    float edgeWidth;

    float edgeIntensity;
    float emissiveIntensity;
    uint enableDissolve;
    uint drawMode;
    
    float4 outlineColor;
    
    float outlineThickness;
    float edgeSoftness;  
    float2 padding;
};

struct SpriteFrameConstants
{
    float4x4 projection; 
};

struct GlobalEnvironmentData
{
    float wetness;
    float rainIntensity;
    float2 windDirection; 
    
    float4 ambientSkyColor;     
    float4 ambientGroundColor;  
    
    float windSpeed;
    float windTurbulence;
    float2 windOffset;
    
    float windTime;
};

struct AtmosphereSkyData
{
    float2 windVelocity;
    float cloudThreshold;        
    float cloudScale;
    
    float cloudShadowBrightness; 
    float skyGradientExponent;
    float sunAtmosphereGlow;
    float cloudBumpScale;
    
    float3 zenithColor;
    float cloudEdgeSoftness;
    
    float3 horizonColor;
    float cloudAbsorption;
    
    float3 sunDirection;
};

struct TrailMaterialData
{
    float jitterStrength;
    float jitterFrequency;
    float jitterSpeed;
    float jitterPhase;
    
    int jitterMode;
    float emissiveIntensity;
    float instanceSeed;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
    
    float4x4 viewProj;
    
    int enable;
    float volumetricScatteringIntensity;
    float2 padding;
};

struct PointLight
{
    float4 color;
    
    float3 position;
    float intensity;
    
    float radius;
    float padding;
    int enable;
    float volumetricScatteringIntensity;
};

struct SpotLight
{
    float4 color;
    
    float3 position;
    float intensity;
    
    float3 direction;
    float distance;
    
    float padding;
    float cosAngle;
    int enable;
    float volumetricScatteringIntensity;
};

struct ParticleInstanceData
{
    float3 position;
    float rotationZ;
    
    float2 scale; 
    float intensity; 
    uint textureAndFlags; 

    float4 color;
};

struct PostEffectData
{
    float pixelationSize;
    float2 screenResolution;
    float totalTime;

    float2 vignetteEllipseScale;
    float vignetteAmount;
    float vignetteRadius;

    float3 vignetteColor;
    float vignetteSoftness;

    float noiseAmount;
    float noiseSpeed;
    float noiseScale;
    int flag;

    float2 chromaOffset;
    float2 radialBlurCenter;

    float radialBlurStrength;
};

struct BrightExtractSettings
{
    float threshold;
    float intensity;
};

struct BloomSettings
{
    float2 texelSize; 
    float radius;
};

struct DoFSettings
{
    float focusDistance;
    float focusRange;
    float transitionRange;
    float bokehRadius;
    
    float bokehHighlightThreshold;
    float bokehHighlightIntensity;
};

struct FinalCompositeSettings
{
    float bloomIntensity;
    int enableDoF;
    int enableSSAO;
    int enableSSR;
    
    float ssrIntensity;
    int enableVolumetricFog; 
    float dofStrength; 
};

struct SSAOSettings
{
    float radius;
    float intensity;
    float bias;
    int sampleCount;

    float fadeStart;
    float fadeEnd;
};

struct BilateralBlurSettings
{
    float2 texelSize;
    float2 direction;
    
    float depthTolerance;
    float normalTolerance;
};

struct InteractionEntity
{
    float3 position; 
    float radius;
    
    float3 velocity; 
    float maxVerticalDist; 
    
    float forceMultiplier;
    float3 padding;
};

struct InteractionConstants
{
    uint entityCount;
    float worldSize;
    float2 centerWorldPos; 
    float2 prevCenterWorldPos;
    
    float2 terrainCenter; 
    float2 terrainSize; 
    
    float trailDuration; 
    float terrainHeightScale; 
    float2 padding;
};

struct GrassInstanceData
{
    float4 posAndHeight; 
    float4 rotationAndWidth;
};

struct GrassMaterialData
{
    float3 rootColor;
    float grassNormalBlend;

    float3 tipColor;
    float sssStrength;

    float3 sssColor;
    float interactStrength;

    float flattenFactor;
    float trailFlattenWeight;
    float windSpeedMultiplier;
    float gustScale;
    
    float gustStrength;
    float flutterAmount;
    float shadowDensity;
    float shadowNormalBias;
    
    float shadowBias;
    float windFlattenStrength;
    float nearFadeMinDist;
    float nearFadeMaxDist;
};

struct GrassCullingData
{
    float maxDrawDistance; 
    float thinStartDistance; 
    float maxThinningRate; 
    float maxWidthMultiplier; 
    
    float lodDistance1; 
    float lodDistance2;
    uint totalInstanceCount;
};

struct GrassGenerationData
{
    float2 terrainCenter;
    float terrainWidth;
    float terrainDepth;

    uint maxGrassCount;
    float gridSpacing;
    float minHeight;
    float maxHeight;
    float minWidth;
    float maxWidth;
};

struct TreeInstanceData
{
    float4x4 worldMatrix;
    float lodFade;  
};

struct TreeInstanceOffset
{
    uint baseInstanceIndex;
    uint isLeaf;
};

struct TreeCullingData
{
    float maxDrawDistance; 
    float approxTreeHeight; 
    float approxTreeRadius; 
    uint totalInstanceCount;
    
    float4 frustumPlanes[6];
};

struct LeafMaterialData
{
    float gustScale;         
    float gustStrength;      
    float trunkFlexibility;  
    float leafFlutterAmount; 

    float diffuseWrap;
    float sssStrength;
    float alphaCutoff;
    float shadowDensity;

    float3 sssColor;
    float shadowNormalBias;

    float3 colorTint;
    float shadowBias;

    float treeHeight;
    float baseRoughness;
    float baseAO;
    float nearFadeMinDist;

    float nearFadeMaxDist;
    float windSpeedMultiplier;  
};

struct TrunkMaterialData
{
    float4 color;

    float roughness;
    float environmentMapIntensity;
    float normalIntensity;
    float shadowDensity;

    float shadowBias;
    float shadowNormalBias;
    float shadowSoftness;
    float nearFadeMinDist;

    float nearFadeMaxDist;
};

struct FoliageInstanceData
{
    float4 posAndScale; 
    float4 rotationQuat;
};

struct FoliageMaterialData
{
    float3 baseColor;
    float alphaCutoff;

    float shadowDensity;
    float shadowBias;
    float shadowNormalBias;
    float windResponse;      

    float plantHeight;       
    float flutterSpeed;      
    float flutterScale;      
    float interactStrength;  

    float trailFlattenWeight;
    float colorIntensity;    
};

struct FoliageGenerationData
{
    float2 terrainCenter;
    float terrainWidth;
    float terrainDepth;
    
    uint maxInstanceCount;
    float gridSpacing;
    float minScale; 
    float maxScale;
};

struct FoliageCullingData
{
    float maxDrawDistance; 
    float thinStartDistance; 
    float maxThinningRate;
    float modelRadius;
    
    float modelCenterYOffset; 
};

struct Object3DInstanceData
{
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float4 WorldColor;
    float4x4 PrevWorld;
};

struct InstanceOffset
{
    int gBaseInstanceIndex;
};

struct VolumetricFogSettings
{
    float3 albedo;         
    float extinctionScale;  
    
    float anisotropy;
    float extinction;      
    float heightDensity;    
    float baseHeight;
    
    float3 ambientLight;
    float heightFalloff;
    
    float temporalWeight;
    float maxDistance;
    float noiseScale;
    float noiseDistortion;
    
    float coverage; 
    float erosion;
    float noiseFeather; 
    float noiseIntensity;
    
    float windSpeedMultiplier;
};

struct FogBilateralSettings
{
    int blurRadius; 
    float spatialSigma; 
    float depthSigma;
};

struct FogVolume
{
    float4x4 worldToLocal;

    float3 color;
    float density;

    float3 noiseScale; 
    float noiseIntensity;

    float3 windDirection; 
    float windSpeed; 

    float coverage; 
    float blendDistance; 
    float erosion; 
    float noiseFeather; 

    float distortionAmount; 
    float heightFalloff; 
    int type; 
    float pad;
};

struct FogVolumeBuffer
{
    FogVolume volumes[MAX_FOG_VOLUMES];
    
    uint volumeCount;
    float3 pad; 
};

struct ShadowData
{
    float4x4 cascadeLightViewProj[MAX_CASCADE_COUNT]; 
    float4 cascadeSplits; 
};

struct CascadeConstant
{
    uint cascadeIndex;
};

struct TerrainSettings
{
    float3 origin;         
    float maxHeight;
    float cellSize;       
    float texelSize;       
    uint heightMapWidth;
    uint heightMapHeight;
};

struct TerrainMaterialData
{
    float4 color;

    float roughness;
    float metalness;
    float environmentMapIntensity;
    float emissiveIntensity;

    int addShadow;
    float shadowBias;
    float shadowNormalBias;
    float shadowDensity;

    float shadowSoftness;
    float shadowEnvStrength;
    int enableNormalMap;
    float normalTiling;

    float normalIntensity;
    float wetness;        
    float rippleScale;     
    float rippleSpeed;

    float rippleStrength;
    float rippleSize;
    float rippleFrequency;
    float padding;
};

struct TerrainChunkData
{
    uint startX;
    uint startZ;
};

struct LightningMaterial
{
    float3 coreColor;
    float coreThickness;  
    
    float3 fringeColor;
    float corePower;
    
    float glowPower;
    float emissiveIntensity;
};

#endif 