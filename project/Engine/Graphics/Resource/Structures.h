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

struct MaterialHandle
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    MaterialData* materialData;

    // マテリアルごとのテクスチャハンドル
    uint32_t textureHandle = 0;       // アルベド
    uint32_t envMapHandle = 0;        // 環境マップ
    uint32_t normalMapHandle = 0;     // 法線マップ
    uint32_t dissolveMapHandle = 0;   // ディゾルブマップ
    uint32_t toonRampHandle = 0;      // トゥーンランプ

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
