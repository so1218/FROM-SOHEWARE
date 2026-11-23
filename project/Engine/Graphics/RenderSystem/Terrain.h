#pragma once
#include "RenderCommon.h"
#include "Structures.h"
#include "TerrainRenderer.h"

namespace FS
{

class Engine;

// ハイトマップの1ピクセルを1頂点とした地形
class Terrain
{
public:
    struct Parameters
    {
        float maxHeight = 20.0f;
        float cellSize = 1.0f;  // 頂点の間隔
    };

    Terrain(Engine* engine);

    void Draw();

    bool LoadFromHeightmap(const std::string& heightmapTextureName, float cellSize);

    // CPU で作った高さから作る
    bool LoadFromHeightRatios(const std::vector<float>& heightRatios, uint32_t width, uint32_t height, float cellSize, float maxHeight);

    // 遠景の地形として描く
    void SetDrawAsVista(bool isVista) { isVista_ = isVista; }
    // 質感と模様の位置を、つながる先の地形にそろえる
    void ShareMaterialFrom(const Terrain* source) { materialSource_ = source; }
    // XZ がこの範囲に収まるチャンクは描かない
    void SetHiddenArea(const Vector2& min, const Vector2& max);

    // 地形の中心の位置
    Vector3& GetPosition() { return position_; }

    // 位置や高さ、ハイトマップが変わるたびに増える
    uint32_t GetShapeVersion() const { return shapeVersion_; }
    void MarkShapeChanged() { ++shapeVersion_; }

    // 地面の高さ
    bool GetHeightAt(float worldX, float worldZ, float& outHeight) const;
    // 地形の外なら 0
    float GetHeight(float worldX, float worldZ) const;

    Parameters& GetParams() { return params_; }
    const Parameters& GetParams() const { return params_; }
    float GetHeightScale() const { return params_.maxHeight; }

    Vector2 GetCenterXZ() const { return { position_.x, position_.z }; }
    Vector2 GetSizeXZ() const;

    std::string* GetHeightmapNamePtr() { return &heightmapTextureName_; }
    const std::string& GetHeightmapName() const { return heightmapTextureName_; }
    uint32_t* GetHeightmapHandlePtr() { return &heightMapHandle_; }

    TerrainMaterial& GetMaterial() { return material_; }

private:
    struct Chunk
    {
        uint32_t startX = 0;
        uint32_t startZ = 0;
        // カリング用
        float minHeightRatio = 0.0f;
        float maxHeightRatio = 0.0f;
    };

    static constexpr uint32_t kChunkCellCount = 64;

    // heightRatios_ と大きさを入れたあとに呼び、チャンクとメッシュを作り直す
    void RebuildFromHeights(uint32_t width, uint32_t height, float cellSize);
    void UploadOwnedHeightMap();
    void GetTexcoordTransform(Vector2& outOrigin, Vector2& outScale) const;

    void CreateChunks();
    void CreateMesh();
    TerrainDrawData CreateDrawData() const;

    float GetHeightRatio(uint32_t x, uint32_t z) const { return heightRatios_[z * heightMapWidth_ + x]; }
    // ハイトマップの (0, 0) のピクセルにあたる頂点の、高さ 0 での位置
    Vector3 GetOrigin() const;

    Engine* engine_ = nullptr;
    TerrainMaterial material_;
    Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Parameters params_;
    uint32_t shapeVersion_ = 0;

    std::string heightmapTextureName_;
    uint32_t heightMapHandle_ = 0;
    uint32_t heightMapWidth_ = 0;
    uint32_t heightMapHeight_ = 0;
    std::vector<float> heightRatios_;

    // 1チャンクのマス数
    uint32_t chunkCellsX_ = 0;
    uint32_t chunkCellsZ_ = 0;
    std::vector<Chunk> chunks_;
    std::vector<TerrainChunkData> visibleChunks_;

    bool isVista_ = false;
    const Terrain* materialSource_ = nullptr;
    bool hasHiddenArea_ = false;
    Vector2 hiddenAreaMin_ = { 0.0f, 0.0f };
    Vector2 hiddenAreaMax_ = { 0.0f, 0.0f };

    // LoadFromHeightRatios で作る高さのテクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource> ownedHeightMap_;
    uint32_t ownedHeightMapSrvIndex_ = 0;
    bool hasOwnedHeightMapSrv_ = false;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    uint32_t indexCount_ = 0;
};

}
