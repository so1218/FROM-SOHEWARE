#ifdef __cplusplus
#include "MathUtils.h" 
#include "WorldTransform.h"

#define float4x4 FE::Matrix4x4
#define float4 FE::Vector4
#define float3 FE::Vector3
#define float2 FE::Vector2
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

#define MAX_DIRECTIONAL_LIGHTS 2
#define MAX_POINT_LIGHTS 10
#define MAX_SPOT_LIGHTS 4
#define MAX_AREA_LIGHTS 2

#define MAX_FOG_EFFECTORS 4
#define MAX_FOG_VOLUMES 8

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
    float padding1;
    
    float3 cameraUp;
    float padding2;

    float3 mainLightDirection;
    float paddingLight0;
    
    float3 mainLightColor;
    float mainLightVolumetricScatteringIntensity;

    float2 iResolution;
    float2 screenResolution;
    
    float gTime;
    float nearClip;
    float farClip;
    float deltaTime;
    
    float3 lightningFlashColor;
    float lightningFlashIntensity;
    
    float4x4 lightViewProj;
    
    float4x4 prevViewProj;
    
    uint32_t frameIndex;
    float3 prevCameraWorldPosition;
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
    float puddleEmission;
    
    int32_t usePuddle;
    float rippleSize;
    float rippleFrequency;
    float rippleLayerMix;
    
    float4 puddleColor;
    
    float puddleTint;
    float3 paddingPuddle;

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
    float treeWindThresholdHeight;
    float2 paddingTree;
    
    int32_t useTriplanar;
    float triplanarScale;
    float triplanarBlendSharpness;
    float paddingTriplanar;
    
    float shadowNormalBias;
    float3 paddingCSM;
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
    float instanceSeed;
    float padding;
    
};

struct WeatherData
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
    
    float3 groundColor;
    float cloudAbsorption;

    float3 cloudAmbientColor; 
    float pad1;
    
    float3 sunDirection;
    float pad2;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
    
    float4x4 viewProj;
    
    int32_t enable;
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
    int32_t enable;
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
    int32_t enable;
    float volumetricScatteringIntensity;
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
    
    int enableSSR;
    float ssrIntensity;
    int enableVolumetricFog; 
    float _padding;
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
    float4 posAndHeight; // xyz: ワールド座標, w: 高さスケール
    float4 rotWidthColor; // x: Y軸回転角, y: 幅スケール, z: パックカラー(uint), w: 予備
};

struct GrassMaterialData
{
    float3 playerPos; 
    float interactStrength; 
    
    float3 rootColor;
    float grassRootAO; 
    
    float3 tipColor;
    float grassNormalBlend; 
    
    float3 sssColor;
    float sssStrength;
    
    float2 windDir;
    float windSpeed;
    float baseWindStrength;
    
    float gustScale; 
    float gustStrength; 
    float flutterAmount; 
    float windHighlightStrength; 
    
    float specularStrength; 
    float specularShininess;
    float wetness; 
    float interactRadius; 
    
    float shadowDensity; 
    float shadowNormalBias; 
    float shadowBias; 
    float colorVariation; 
    
    float windFlattenStrength;
};

struct GrassCullingData
{
    float4 frustumPlanes[6]; 
    
    float maxDrawDistance; 
    float thinStartDistance; 
    float maxThinningRate; 
    float maxWidthMultiplier; 
    
    float lodDistance1; 
    float lodDistance2;
    uint32_t totalInstanceCount;
};

struct GrassGenerationData
{
    float2 chunkBasePos;
    float2 terrainCenter;
    
    float terrainWidth;
    float terrainDepth;
    uint32_t maxGrassPerChunk;
    float gridSpacing;
  
    float minHeight;
    float maxHeight;
    float minWidth;
    float maxWidth;
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
    float pad2;
    
    float noiseDistortion;
    float windSpeed;
    float coverage; 
    float worleyWeight; 
    
    float erosion;
    float noiseFeather; 
    float erosionStrength;
    float noiseIntensity;
    
    float3 windDirection;
    float pad3;
};

struct FogBilateralSettings
{
    int blurRadius; 
    float spatialSigma; 
    float depthSigma;
    float padding;
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
    
    uint32_t volumeCount;
    float3 pad; 
};

struct FluidSettings
{
    float velocityDissipation; 
    float densityDissipation;
    float gridScale;
    float vorticityStrength;


    float3 gridMin; 
    float interactionRadius; 

    float3 gridMax; 
    float injectionStrength;

    float3 objectPos; 
    float densityAmount; 

    float3 objectVelocity; 
    float dragStrength; 

    float pushStrength;
    float uvwRelaxation; 
    float2 paddingFluid3;
    
    float3 voxelDelta;
    float pad1;
};

struct ShadowData
{
    float4x4 cascadeLightViewProj[4]; // 4枚分のカスケード行列
    float4 cascadeSplits; // カスケードの切り替わり距離 (x, y, z, w)
};

struct CascadeConstant
{
    uint32_t cascadeIndex;
};

struct TerrainSettings
{
    float maxHeight; 
    float texelSize;
    float cellSize;
    float padding;
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
    float2 padding;
};