#pragma once
#include "MathUtils.h"
#include "WorldTransform.h"
#include "ShaderConstants.hlsli"

#include <span>

namespace FE
{

struct VertexData
{
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
    Vector3 tangent;
    Vector3 smoothNormal;
    Vector4 color;
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
    Vector4 color;
};

struct TerrainVertexData 
{
    Vector4 position; 
    Vector2 texcoord; 
};

struct LightningVertex
{
    Vector4 position;
    Vector2 texcoord;
    Vector4 color;
};

struct MaterialHandle
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    MaterialData* materialData;

    std::string textureName = "white1x1";       // アルベド
    std::string envMapName = "skybox";  // 環境マップ
    std::string normalMapName = "white1x1";       // 法線マップ
    std::string heightMapName = "white1x1";       // POMハイトマップ
    std::string dissolveMapName = "white1x1";       // ディゾルブマップ
    std::string toonRampName = "toonRamp_01";       // トゥーンランプ
    std::string rippleTextureName = "white1x1";     // 波紋用テクスチャ
    std::string puddleNoiseName = "white1x1";        // 水たまり用ノイズ

    // マテリアルごとのテクスチャハンドル
    uint32_t textureHandle = 0;     
    uint32_t envMapHandle = 0;      
    uint32_t normalMapHandle = 0;   
    uint32_t heightMapHandle = 0;
    uint32_t dissolveMapHandle = 0; 
    uint32_t toonRampHandle = 0;    
    uint32_t rippleTextureHandle = 0;
    uint32_t puddleNoiseHandle = 0;

    // エディタ編集用UVデータ
    WorldTransform uvTransformData;
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
    std::vector<unsigned int> meshIndices; // このノードが持つメッシュIDのリスト
};

struct MeshData
{
    // 形状データ
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;

    // マテリアル・テクスチャ情報（パーツごとに異なるため）
    MaterialHandle materialHandle;
    TextureData textureData;

    // スキニング情報（このメッシュの頂点に対するウェイト）
    std::map<std::string, JointWeightData> skinClusterData;
};

struct ModelData
{
    // 複数のメッシュ（パーツ）を持つリストに変更
    std::vector<MeshData> meshes;

    // スケルトン階層はモデル全体で1つ共有
    Node rootNode;
};

enum class CullMode
{
    Back,   // 通常 (裏面カリング)
    Front,  // 前面カリング
    None    // カリングなし (両面描画)
};

enum class DepthMode
{
    Write,      // 書き込みあり (通常)
    ReadOnly,   // 書き込みなし・テストあり (半透明・エフェクト)
    None        // テストも書き込みもなし (UI・常に最前面)
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

    int32_t modeFlags[2];
    float2 _paddingGlow2;

    float dissolveThreshold;
    float dissolveEdgeWidth;
    float dissolveEdgeIntensity;
    float _paddingDissolve;

    float3 dissolveEdgeColor;
    float _paddingDissolve2;

    float radialBlurStrength;
    float2 radialBlurCenter;
    float _paddingRadial;
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

// シェーダーに送る雷の見た目パラメータ
struct LightningConfig
{
    Vector3 coreColor = { 1.0f, 1.0f, 1.0f };       // 芯の色
    float coreThickness = 0.15f;                    // 芯の太さ
    Vector3 fringeColor = { 0.1f, 0.5f, 1.0f };     // 滲みの色
    float corePower = 4.0f;                         // 芯の鋭さ
    float glowPower = 2.5f;                         // 滲みの広がり
    float emissiveIntensity = 50.0f;                // ブルームの強さ
    float flickerSpeed = 60.0f;                     // 明滅スピード
    float displacement = 20.0f;                     // ジグザグの荒ぶる幅
    float durationMin = 0.08f;                      // 最小表示時間
    float durationMax = 0.35f;                      // 最大表示時間
    float lightIntensityMax = 150.0f;               // 落雷時の最大輝度（かなり強めが良い）
    float lightRadius = 100.0f;                     // 光の届く範囲（メートル）
    float lightHeightOffset = 10.0f;                // 落雷地点(地面)からどれくらい浮かせた位置にライトを置くか
    float volumetricScattering = 5.0f;              // ボリュメトリックフォグへの影響（空気を光らせる）
    Vector4 lightColor = { 0.7f, 0.85f, 1.0f, 1.0f };
    float thickness = 0.5f;      // メッシュ自体の太さ(ワールド空間)
    float flickerMin = 0.4f;     // 明滅の最も暗い時の倍率
    float flickerMax = 1.2f;     // 明滅の最も明るい時の倍率
    int fractalDepth = 6;        // ジグザグの分割数（6=64分割、7=128分割）
    float branchProbability = 0.3f; // 枝分かれが発生する確率 (0.0〜1.0)
    float branchLengthScale = 0.6f; // 枝の長さ（本流に対する割合）
};

}

// ハッシュ関数の定義
namespace std
{

template <>
struct hash<FE::VertexKey>
{
    size_t operator()(const FE::VertexKey& key) const {
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

