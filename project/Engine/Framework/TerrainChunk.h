#pragma once
#include "Structures.h"

namespace FE
{
class Engine;

class TerrainChunk
{
public:
    TerrainChunk(Engine* engine, int startX, int startZ, int numCellsX, int numCellsZ, float cellSize, float offsetX, float offsetZ);

    float GetHeightAt(float worldX, float worldZ) const;

    // ゲッター群
    uint32_t GetHeightmapTextureHandle() const { return heightmapTextureHandle_; }
    uint32_t GetGroundTextureHandle() const { return groundTextureHandle_; }

    // 描画用メッシュを生成する関数（LoadFromHeightmapの最後で呼ぶ）
    bool CreateMesh();

    // 親の Terrain クラスから、抽出した高さデータを流し込んでもらう
    void SetHeightData(const std::vector<float>& localHeightData);

    // カリング用の AABB の最小・最大座標
    Vector3 GetAABBMin() const { return aabbMin_; }
    Vector3 GetAABBMax() const { return aabbMax_; }

    // 描画用ゲッター
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vbView_; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return ibView_; }
    UINT GetIndexCount() const { return indexCount_; }

    void SetUVScale(float scale) { uvScale_ = scale; }

    int GetStartX() const { return startX_; }
    int GetStartZ() const { return startZ_; }
    int GetNumCellsX() const { return numCellsX_; }
    int GetNumCellsZ() const { return numCellsZ_; }

private:
    Engine* engine_;

    int numCellsX_;
    int numCellsZ_;
    float cellSize_;
    std::vector<float> heightData_;

    // ハンドルで管理
    uint32_t heightmapTextureHandle_ = 0; // ハイトマップ自体のハンドル（デバッグ表示等用）
    uint32_t groundTextureHandle_ = 0;    // 地面に貼り付ける草や岩のテクスチャハンドル

    // リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};
    UINT indexCount_ = 0;

    int startX_; // 全体マップにおけるXの開始オフセット
    int startZ_; // 全体マップにおけるZの開始オフセット

    float offsetX_ = 0.0f;
    float offsetZ_ = 0.0f;

    Vector3 aabbMin_;
    Vector3 aabbMax_;

    // 法線を自動計算するヘルパー関数
    void ComputeNormals(std::vector<VertexData>& vertices, int numVertsX, int numVertsZ);

private:
    float uvScale_ = 0.1f; // 追加
};

}