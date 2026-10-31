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
#define COLOR_TINT          (1 << 1)
#define VIGNETTE            (1 << 2)
#define SCREEN_NOISE        (1 << 3)
#define CHROM_ABERRATION    (1 << 4)
#define RADIAL_BLUR         (1 << 5)
#define COLOR_GRADING_LUT   (1 << 6)

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
    float gTime;
    
    float3 mainLightColor;
    float mainLightVolumetricScatteringIntensity;

    float2 iResolution;
    float2 screenResolution;
    
    float4x4 lightViewProj;
    
    float4x4 prevViewProj;
    
    uint frameIndex;
    float3 prevCameraWorldPosition;
    
    float deltaTime;
    float3 pad;
    
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
    int enableRipple;
    float wetness;
    
    float rippleScale;
    float rippleSpeed;
    float rippleStrength;
    float rippleSize;
    
    float rippleFrequency;
    float rippleLayerMix;
    int isBubble;
    float wobbleSpeed;
    
    float wobbleAmplitude;
    float rainbowIntensity;
    float fresnelExponent;
    float shadowNormalBias;
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
    float3 padding;
};

struct SpriteFrameConstants
{
    float4x4 projection; // 左上原点の正射影行列
};

struct GlobalEnvironmentData
{
    float wetness;
    float rainIntensity;
    float2 windDirection; 
    
    float4 skyColor; 
    float4 groundColor;
    
    float windSpeed;
    float windTurbulence;
    float2 windOffset;
    
    float windTime;
};

struct AtmosphereSkyData
{
    float2 cloudCoverage; 
    float2 windVelocity;
    
    float cloudScale;
    float cloudShadowDensity; 
    float skyGradientExponent;
    float sunAtmosphereGlow;
    
    float3 zenithColor;
    float cloudBumpScale;
    
    float3 horizonColor;
    float cloudEdgeSoftness;
    
    float cloudAbsorption;
    float3 sunDirection;
};

struct WaterMaterialData
{
    float2 globalWindDirection; 
    float waveLength; 
    float waveAmplitude; 

    float waveSteepness; 
    float waveSpeed;
    float waveAmplitudeFalloff;
    float waveLengthFalloff;
    
    float waveDirectionSpread; 
    float interactionHeightScale; 
    float interactionSinkForce; 
    float interactionBulgeForce; 

    float interactionNormalScale; 
    float interactionFoamIntensity; 
    float2 normalTiling; 
    
    float4 shallowColor; 
    float4 deepColor;
    float4 scatterColor; 
    float4 foamColor;
    
    float absorption; 
    float refractionAmount; 
    float chromaticAberration; 
    float normalIntensity; 

    float roughness; 
    float specularIntensity; 
    float envReflectionIntensity;
    float ssrIntensity; 
    
    float ssrStepSize; 
    float ssrMaxDistance;
    float ssrThickness; 
    float waveFoamThreshold; 
    
    float shoreFoamThreshold; 
    float foamScale; 
    float foamIntensity; 
    float ssrDistortion; 

    float ssrMaxSteps; 
    float ssrBinarySearchSteps; 
    float2 pad0; 
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
    
    float3 tintColor;
    float tintMulColorAmount;
    
    float tintAddColorAmount;
    float tintScreenColorAmount;
    float2 vignetteEllipseScale;

    float vignetteAmount;
    float vignetteRadius;
    float vignetteSoftness;
    float padding_1;
    
    float3 vignetteColor;
    float noiseAmount;
    
    float noiseSpeed;
    float noiseScale;
    float2 chromaOffset;

    int flag; 
    float radialBlurStrength;
    float2 radialBlurCenter; 
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
    float3 position; // ワールド位置
    float radius; // 影響半径
    
    float3 velocity; // 移動速度ベクトル
    float maxVerticalDist; // 影響を及ぼす最大垂直距離
    
    float forceMultiplier; // 力の倍率
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
    float4 posAndHeight; // xyz: ワールド座標, w: 高さスケール
    float4 rotWidthColor; // x: Y軸回転角, y: 幅スケール, z: パックカラー, w: 予備
};

struct GrassMaterialData
{
    float3 rootColor;
    float grassRootAO; 
    
    float3 tipColor;
    float grassNormalBlend; 
    
    float3 sssColor;
    float sssStrength;
    
    float interactStrength;
    float flattenFactor;
    float trailFlattenWeight;
    float windSpeedMultiplier; 
    
    float windStrengthMultiplier;
    float gustScale; 
    float gustStrength; 
    float flutterAmount; 
    
    float windHighlightStrength; 
    float specularStrength; 
    float specularShininess;
    float wetness; 
    
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
    float2 chunkBasePos;
    float2 terrainCenter;
    
    float terrainWidth;
    float terrainDepth;
    uint maxGrassPerChunk;
    float gridSpacing;
  
    float minHeight;
    float maxHeight;
    float minWidth;
    float maxWidth;
};

struct TreeInstanceData
{
    float4x4 worldMatrix;
    float lodFade;   // カリングの CS が距離から決める
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
    float gustScale;          // 突風ノイズの細かさ
    float gustStrength;       // 突風の強さ
    float trunkFlexibility;   // 幹のしなりやすさ
    float leafFlutterAmount;  // 葉の揺れ幅

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
    float windSpeedMultiplier;  // 環境の風に対する揺れの速さ
};

struct TrunkMaterialData
{
    float4 color;
    float4 specularColor;

    int enableLighting;
    int lightMode;
    int enableNormalMap;
    int addShadow;

    float roughness;
    float metalness;
    float shininess;
    float diffuseReflection;

    float shadowDensity;
    float shadowBias;
    float shadowNormalBias;
    float shadowSoftness;

    float environmentMapIntensity;
    float shadowEnvStrength;
    float normalIntensity;

    float nearFadeMinDist;
    float nearFadeMaxDist;
};

struct FoliageInstanceData
{
    float4 posAndScale;   // xyz = 根元の位置, w = 大きさ
    float4 rotationQuat;
};

struct FoliageMaterialData
{
    float3 baseColor;
    float alphaCutoff;

    float shadowDensity;
    float shadowBias;
    float shadowNormalBias;
    float windResponse;       // 風で揺れる大きさ

    float plantHeight;        // この高さで揺れが最大になる
    float flutterSpeed;       // 細かい揺れの速さ
    float flutterScale;       // 細かい揺れの大きさ
    float interactStrength;   // 押されて倒れる大きさ

    float trailFlattenWeight; // 通った跡が残る強さ
    float colorIntensity;     // 明るさの倍率 (baseColor は 0〜1 なので、明るくするときに使う)
};

struct FoliageGenerationData
{
    float2 terrainCenter;
    float terrainWidth;
    float terrainDepth;
    
    uint maxInstancesPerChunk;
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
    uint totalInstanceCount;
    float2 padding;
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
    float scatteringIntensity;
    
    float extinctionScale;
    float anisotropy;
    float extinction;
    float heightDensity;
    
    float baseHeight;
    float heightFalloff;
    float2 pad1; 
    
    float3 ambientLight;
    float temporalWeight;
    
    float maxDistance;
    float depthSliceCount;
    float noiseScale;
    float noiseDistortion;
    
    float coverage; 
    float worleyWeight; 
    float erosion;
    float noiseFeather; 
    
    float erosionStrength;
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
    float anisotropy; 
    float blendDistance; 
    int type; 

    float worleyWeight; 
    float erosion; 
    float noiseFeather; 
    float distortionAmount; 

    float densityOffset; 
    float noiseContrast; 
    float heightFalloff; 
    float pad0; 
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
    float maxHeight; 
    float texelSize;
    float cellSize;
};

struct TerrainInstanceData
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float4 WorldColor;
    float4 uvTransform;
};

struct LightningMaterial
{
    float3 coreColor; 
    float coreThickness;
    
    float3 fringeColor; 
    float corePower;
    
    float glowPower; 
    float flickerSpeed; 
    float flickerMin; 
    float flickerMax; 
    
    float emissiveIntensity; 
    float instanceSeed;
};

#endif 