#pragma once
#include "MathUtils.h"
#include "WorldTransform.h"
#include "UVTransform.h"
#include "Common/ShaderConstants.hlsli"

namespace FS
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

struct TrailVertexData
{
    Vector4 position;
    Vector2 texcoord;
    Vector4 color;
};

struct LineVertexData
{
    Vector4 position;
    Vector4 color;
};

// チャンクの中で何番目の頂点か。位置と高さはシェーダーで決める
struct TerrainVertexData
{
    uint32_t gridX;
    uint32_t gridZ;
};


struct LightningVertexData
{
    Vector4 position;
    Vector2 texcoord;
    float brightness; // 消えていく分と明滅を掛けた明るさ
};

struct TextureData
{
    std::string textureFilePath;
    uint32_t textureHandle = 0;
};

struct AABB
{
    Vector3 min;
    Vector3 max;
};

struct MaterialHandle
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    MaterialData* materialData;

    std::string textureName = "white1x1";      
    std::string envMapName = "pureSky";  
    std::string normalMapName = "white1x1";       
    std::string dissolveMapName = "white1x1";      
    std::string toonRampName = "toonRamp_01";      

    // マテリアルごとのテクスチャハンドル
    uint32_t textureHandle = 0;     
    uint32_t envMapHandle = 0;      
    uint32_t normalMapHandle = 0;  
    uint32_t dissolveMapHandle = 0; 
    uint32_t toonRampHandle = 0;    

    // エディタ編集用UVデータ
    UVTransform uvTransformData;
};

// 地形専用のマテリアル。MaterialHandle と違い、地形が使う値とテクスチャだけを持つ
struct TerrainMaterial
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    TerrainMaterialData* data = nullptr;

    std::string albedoMapName = "white1x1";
    std::string environmentMapName = "pureSky";
    std::string normalMapName = "white1x1";
    std::string rippleMapName = "white1x1";

    uint32_t albedoMapHandle = 0;
    uint32_t environmentMapHandle = 0;
    uint32_t normalMapHandle = 0;
    uint32_t rippleMapHandle = 0;
};

struct TreeMaterialHandle
{
    Microsoft::WRL::ComPtr<ID3D12Resource> leafMaterialBuffer; 
    LeafMaterialData* mappedLeafData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> trunkMaterialBuffer; 
    TrunkMaterialData* mappedTrunkData = nullptr;

    uint32_t trunkTextureHandle = 0;       
    uint32_t trunkNormalMapHandle = 0;    
    uint32_t leafTextureHandle = 0;       
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

    AABB localAABB;
};

struct ModelData
{
    // 複数のメッシュを持つリストに変更
    std::vector<MeshData> meshes;

    // デフォルトの共有マテリアル
    std::vector<MaterialHandle> defaultMaterials;

    // スケルトン階層はモデル全体で1つ共有
    Node rootNode;
};

enum class CullMode
{
    Back,   // 裏面カリング
    Front,  // 前面カリング
    None    // カリングなし
};

enum class DepthMode
{
    Write,      // 書き込みあり (通常)
    ReadOnly,   // 書き込みなし・テストあり (半透明・エフェクト)
    None        // テストも書き込みもなし (UI・常に最前面)
};

// 雷の見た目・形・寿命・光の設定
struct LightningConfig
{
    // 見た目
    Vector3 coreColor = { 1.0f, 1.0f, 1.0f };
    Vector3 fringeColor = { 0.1f, 0.5f, 1.0f };
    float thickness = 0.5f;
    float coreThickness = 0.15f;
    float corePower = 4.0f;
    float glowPower = 2.5f;
    float emissiveIntensity = 50.0f;

    // 形
    int fractalDepth = 6;
    float displacement = 20.0f;
    float branchProbability = 0.3f;
    float branchLengthScale = 0.6f;

    // 寿命と明滅
    float durationMin = 0.08f;
    float durationMax = 0.35f;
    float flickerSpeed = 60.0f;
    float flickerMin = 0.4f;
    float flickerMax = 1.2f;

    // 落ちた場所を照らす光
    Vector4 lightColor = { 0.7f, 0.85f, 1.0f, 1.0f };
    float lightIntensityMax = 150.0f;
    float lightRadius = 100.0f;
    float lightHeightOffset = 10.0f;
    float volumetricScattering = 5.0f;
};

// 雷の1本の線。幹と枝それぞれが1本ずつ
struct LightningPath
{
    std::vector<Vector3> points;
    float thicknessScale = 1.0f; // 枝は幹より細くする
};

struct FoliageLayer
{
    std::string name; 

    std::string modelName;
    std::string albedoName = "white1x1";
    uint32_t albedoHandle = 0;

    std::string densityMapName = "white1x1"; 
    uint32_t densityMapHandle = 0;

    FoliageMaterialData material{};
    FoliageGenerationData genData{};

    // エディターで決める上限。実際の数は範囲を埋めるのに必要な数と比べて小さいほう
    int uiMaxInstances = 50000;
};

}
