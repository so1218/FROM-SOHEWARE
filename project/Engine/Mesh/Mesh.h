#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include "BufferManager.h" 
#include "Structures.h"    

class Mesh
{
public:
    // 初期化(頂点のみ)
    void InitializeVertexOnly(ID3D12Device* device, const std::vector<VertexData>& vertices);

    // 初期化(頂点 + インデックス)
    void Initialize(ID3D12Device* device, const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indies);

    // Trail用（頂点カラー付き）の初期化関数
    void InitializeVertexTrail(ID3D12Device* device, const std::vector<VertexDataTrail>& vertices);

    void CreateDynamicMesh(ID3D12Device* device, size_t maxVertexCount, size_t stride);

    // ゲッター
    size_t GetVertexCount() const { return vertexCount_; }
    size_t GetIndexCount() const { return indexCount_; }
    VertexData* GetVertexData() const { return vertexData_; }

    // GPUリソース取得
    ID3D12Resource* GetVertexResource() const { return vertexResource_.Get(); }
    ID3D12Resource* GetIndexResource() const { return indexResource_.Get(); }

    // ビュー取得
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBufferView_; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBufferView_; }

    // セッター
    void SetVertexCount(size_t count) { vertexCount_ = count; }
    void SetIndexCount(size_t count) { indexCount_ = count; }

private:
    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

    // バッファビュー
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    // その他のデータ
    VertexData* vertexData_ = nullptr;
    size_t vertexCount_ = 0;
    size_t indexCount_ = 0;
};
