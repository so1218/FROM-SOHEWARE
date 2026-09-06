#ifndef SHADER_CONSTANTS_HLSLI
#define SHADER_CONSTANTS_HLSLI

#ifdef __cplusplus
#include "MathUtils.h" 
#include "WorldTransform.h"

#define float4x4 FE::Matrix4x4
#define float4 FE::Vector4
#define float3 FE::Vector3
#define float2 FE::Vector2
#define float32_t float
#define uint uint32_t

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

// シャドウマップ
#define MAX_CASCADE_COUNT 4
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
    float puddleScale;
    
    float puddleFalloff;
    float puddleEmission;
    int usePuddle;
    float rippleSize;
    
    float4 puddleColor;
    
    float rippleFrequency;
    float rippleLayerMix;
    float puddleTint;
    int isBubble;
    
    float wobbleSpeed;
    float wobbleAmplitude;
    float rainbowIntensity;
    float fresnelExponent;
   
    int useTriplanar;
    float triplanarScale;
    float triplanarBlendSharpness;
    float shadowNormalBias;
    
    int enablePOM;
    float pomHeightScale;
    float pomMinSteps; 
    float pomMaxSteps; 
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
    
    float3 groundColor;
    float cloudAbsorption;

    float3 cloudAmbientColor; 
    float pad1;
    
    float3 sunDirection;
};

struct WaterMaterialData
{
// --------------------------------------------------------
    // 波のグローバル設定
    // --------------------------------------------------------
    float2 windDirection; // 風向き [1.0, 0.5]
    float baseWaveLength; // 基本波長
    float baseAmplitude; // 基本振幅

    float baseSteepness; // 波の鋭さ
    float waveSpeed; // 進行速度
    float wavePersistence; // 小波減衰率
    float waveLacunarity; // 小波周波数倍率

    float waveDirectionSpread; // 子波拡散角度
    float waveChop; // 水平引き寄せ
    float normalIntensity; // ★ 追加: 法線マップの適用強度 (0.0〜1.0)
    float waveFoamThreshold; // ★ 追加: 波頭の泡の発生しきい値 (0.0〜1.0)

    // --------------------------------------------------------
    // カラー設定
    // --------------------------------------------------------
    float4 shallowColor; // 浅瀬の色
    float4 deepColor; // 深い場所の色
    float4 scatterColor; // 水中散乱光
    float4 foamColor; // 泡の色

    // --------------------------------------------------------
    // ライティング・光学設定
    // --------------------------------------------------------
    float absorption; // 吸光度
    float refractionAmount; // 屈折強度
    float2 waveTiling; // 法線タイリング

    float roughness; // ラフネス
    float specularIntensity; // ハイライト強度
    float envReflectionIntensity; // 環境マップ強度
    float causticsScale; // コースティクスサイズ

    // --------------------------------------------------------
    // エフェクト設定
    // --------------------------------------------------------
    float causticsIntensity; // コースティクス強度
    float causticsFadeDepth; // コースティクス消滅深度
    float chromaticAberration; // 色収差強度
    float foamScale; // 泡ノイズのサイズ

    float foamThreshold; // 岸辺の泡の範囲 (水深)
    float foamIntensity; // 泡の濃さ
    float rainIntensity; // 雨の強度
    float rippleScale; // 波紋サイズ

    float rippleSpeed; // 波紋速度
    float rippleStrength; // 波紋強度
    float ssrIntensity; // SSR強度
    float ssrThickness; // SSR交差厚み

    float ssrStepSize; // SSRレイ初期ステップ幅
    float ssrMaxDistance; // SSR最大距離
    float causticsSpeed; // コースティクス揺らぎ速度
    float causticsDistortion; // コースティクス屈折歪み
    
    float interactionHeightScale; // 頂点変形全体のスケール
    float interactionSinkForce; // 足元の沈み込み強度
    float interactionBulgeForce; // 周囲波の盛り上がり強度
    float interactionNormalScale; // 波紋法線歪み強度

    float interactionFoamIntensity; // 移動痕跡の泡強度
    float3 pad0; 
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

struct AreaLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 right;
    float range;
    float3 up;
    float decay;
    int enable;
    float3 padding;
};

struct ParticleInstanceData
{
    float3 position;
    float rotationZ;
    
    float2 scale;
    uint textureIndex;
    int isBillboard;
    
    float4 color;
    
    float intensity;
    float3 padding;
};

struct PostEffectData
{
    float pixelationSize;
    float3 _padding0;

    float2 screenResolution;
    float grayscaleColorAmount;
    float sepiaColorAmount;

    float tintMulColorAmount;
    float tintAddColorAmount;
    float tintScreenColorAmount;
    float _padding1;

    float3 tintColor;
    float totalTime;

    float vignetteAmount;
    float vignetteRadius;
    float vignetteSoftness;
    float padding_1;

    float2 vignetteEllipseScale;
    float2 padding_2;

    float noiseAmount;
    float noiseSpeed;
    float noiseScale;
    float _padding2;

    float chromaOffset;
    float waveAmplitude;
    float waveFrequency;
    float _paddingWave;

    int waveDirection;
    float waveSpeed;
    float fisheyeDistortion;
    float _paddingFisheye;

    float scanlineIntensity;
    float scanlineFrequency;
    int scanlineDirection;
    float _padding3;

    float3 scanlineColor;
    float scanlineScrollSpeed;

    float blockNoiseAmount;
    float blockNoiseSize;
    float blockNoiseSpeed;
    float _padding4;

    float rgbSplitOffset;
    float filmGrainIntensity;
    float _padding5;
    float _padding6;

    float glitchBlockHeight;
    float glitchAmount;
    float glitchNoiseIntensity;
    float _padding7;

    float heatDistortionStrength;
    float heatSpeed;
    float heatNoiseScale;
    float _padding8;

    float3 vignetteColor;
    float _padding9;

    float turbulentStrength;
    float turbulentFrequency;
    float turbulentSpeed;
    float _paddingTurbulence;

    int flag[2];
    float2 _paddingGlow2;

    float dissolveThreshold;
    float dissolveEdgeWidth;
    float dissolveEdgeIntensity;
    float _paddingDissolve;

    float3 dissolveEdgeColor;
    float _paddingDissolve2;

    float radialBlurStrength;
    float2 radialBlurCenter;
};

struct BrightExtractSettings
{
    float threshold;
    float intensity;
};

struct BlurSettings
{
    float2 texelSize;
    float blurStrength;
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

struct SSRSettings
{
    float maxDistance;
    float stepSize;
    int maxSteps;
    float thickness;
};

struct InteractionEntity
{
    float3 position; // ワールド位置
    float radius; // 影響半径
    
    float3 velocity; // 移動速度ベクトル
    float maxVerticalDist; // 影響を及ぼす最大垂直距離
    
    uint entityType; // 0: 人間, 1: 大型/車両, 2: 衝撃波/爆発
    float forceMultiplier; // 力の倍率
    float2 padding;
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
    float4 rotWidthColor; // x: Y軸回転角, y: 幅スケール, z: パックカラー(uint), w: 予備
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
    float colorVariation; 
    
    float windFlattenStrength;
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
    float4 colorVariation; 
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
    float windSpeedMultiplier; 
    float windStrengthMultiplier;
    float gustScale;
    float gustStrength; 
    
    float trunkFlexibility;
    float branchFlexibility; 
    float leafFlutterAmount; 
    float backfaceFlatten; 
    
    float diffuseWrap; 
    float transmissionDistortion; 
    float transmissionPower; 
    float sssStrength; 
    
    float alphaCutoff; 
    float shadowDensity; 
    float shadowNormalBias;
    float treeHeight;
    
    float3 sssColor; 
    float treeRadius; 
    
    float shadowBias; 
    float baseRoughness;
    float baseAO; 
    float baseThickness;
    
    float albedoMultiplier;
    float3 colorTint; 
    
    float leafFlutterFrequency;
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
    float albedoMultiplier;
};

struct PebbleInstanceData
{
    float4 posAndScale; // xyz: ワールド座標, w: スケール
    float4 rotationQuat; // x, y, z, w: 姿勢(クォータニオン)
    float4 anisoAndEmbed; // x, y, z: 非等方スケール比率, w: 埋まり具合 0.0~1.0
    float3 colorVariation; // RGB 色ムラ
    float padding;
};

struct PebbleMaterialData
{
    float4 baseColor;
    
    float roughness;
    float metalness;
    float normalIntensity;
    float shadowDensity;
    
    float shadowNormalBias;
    float shadowBias;
    float shadowSoftness;
    float shadowEnvStrength;
    
    float environmentMapIntensity;
    float shininess;
    float diffuseReflection;
    float padding;
};

struct PebbleGenerationData
{
    float2 chunkBasePos;
    float2 terrainCenter;
    
    float terrainWidth;
    float terrainDepth;
    uint maxInstancesPerChunk;
    float gridSpacing;
    
    float minScale;
    float maxScale; 
    float2 heightMapTexelSize;
  
    float3 minAnisoScale; 
    float padding1;
    
    float3 maxAnisoScale; 
    float padding2;
};

struct PebbleCullingData
{
    float maxDrawDistance;
    float thinStartDistance;
    float maxThinningRate;
    float modelRadius; 
    
    float modelCenterYOffset; 
    uint totalInstanceCount;
    float2 padding;
};

struct FoliageInstanceData
{
    float4 posAndScale;
    float4 rotationQuat;
    float3 colorVariation;
    uint padding;
};

struct FoliageMaterialData
{
    float3 baseColor;
    float roughness;
    
    float alphaCutoff;
    float sssStrength;
    float windResponse; 
    float stiffness; 
    
    float flutterSpeed; 
    float flutterScale; 
    float plantHeight;
    float shadowDensity;
    
    float shadowBias;
    float shadowNormalBias;
    float interactStrength; 
    float flattenFactor; 
    
    float trailFlattenWeight;
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
    float4x4 cascadeLightViewProj[MAX_CASCADE_COUNT]; // MAX_CASCADE_COUNT 枚分のカスケード行列
    float4 cascadeSplits; // カスケードの切り替わり距離
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