#include "pch.h"
#include "TerrainChunk.h"
#include "TextureManager.h"
#include "TextureLoader.h"
#include "Engine.h"
#include "BufferManager.h"

namespace FE
{

TerrainChunk::TerrainChunk(Engine* engine, int startX, int startZ, int numCellsX, int numCellsZ, float cellSize, float offsetX, float offsetZ,
    float totalVertsX, float totalVertsZ)
    : engine_(engine), startX_(startX), startZ_(startZ), numCellsX_(numCellsX), numCellsZ_(numCellsZ), cellSize_(cellSize), offsetX_(offsetX), offsetZ_(offsetZ),
    totalVertsX_(totalVertsX), totalVertsZ_(totalVertsZ)
{
    int numVertsX = numCellsX_ + 1;
    int numVertsZ = numCellsZ_ + 1;
    heightData_.resize(numVertsX * numVertsZ, 0.0f);
}

bool TerrainChunk::GetHeightAt(float targetX, float targetZ, float& outHeight) const
{
    float chunkLocalX = (targetX + offsetX_) - (startX_ * cellSize_);
    float chunkLocalZ = (targetZ + offsetZ_) - (startZ_ * cellSize_);

    float localX = chunkLocalX / cellSize_;
    float localZ = chunkLocalZ / cellSize_;

    int cellX = static_cast<int>(std::floor(localX));
    int cellZ = static_cast<int>(std::floor(localZ));

    // 範囲外の場合は 0 ではなく false を返す
    if (cellX < 0 || cellX >= numCellsX_ || cellZ < 0 || cellZ >= numCellsZ_) {
        return false;
    }

    float u = localX - cellX;
    float v = localZ - cellZ;

    int numVertsX = numCellsX_ + 1;
    float h00 = heightData_[cellZ * numVertsX + cellX];
    float h10 = heightData_[cellZ * numVertsX + (cellX + 1)];
    float h01 = heightData_[(cellZ + 1) * numVertsX + cellX];
    float h11 = heightData_[(cellZ + 1) * numVertsX + (cellX + 1)];

    if (u + v <= 1.0f)
    {
        outHeight = h00 + (h10 - h00) * u + (h01 - h00) * v;
    }
    else
    {
        outHeight = h11 + (h01 - h11) * (1.0f - u) + (h10 - h11) * (1.0f - v);
    }

    return true;
}

bool TerrainChunk::CreateMesh()
{
    if (vertexBuffer_ && indexBuffer_) return true;

    int numVertsX = numCellsX_ + 1;
    int numVertsZ = numCellsZ_ + 1;
    size_t vertexCount = numVertsX * numVertsZ;

    std::vector<TerrainVertexData> vertices(vertexCount);

    float maxGlobalW = static_cast<float>(totalVertsX_);
    float maxGlobalH = static_cast<float>(totalVertsZ_);

    // ループ前にAABBを初期化
    aabbMin_ = { FLT_MAX, FLT_MAX, FLT_MAX };
    aabbMax_ = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (int z = 0; z < numVertsZ; ++z)
    {
        for (int x = 0; x < numVertsX; ++x)
        {
            int index = z * numVertsX + x;

            float localX = (startX_ + x) * cellSize_ - offsetX_;
            float localZ = (startZ_ + z) * cellSize_ - offsetZ_;

            // 頂点バッファのY座標自体はShaderで上げるので0のまま
            float localY = 0.0f;
            vertices[index].position = { localX, localY, localZ, 1.0f };

            float globalX = static_cast<float>(startX_ + x);
            float globalZ = static_cast<float>(startZ_ + z);

            vertices[index].texcoord = {
                (globalX + 0.5f) / maxGlobalW,
                (globalZ + 0.5f) / maxGlobalH
            };

            // AABBの高さを実際の地形データから取得する
            // CPU側での視界判定用に、箱の高さを実際の地形で更新
            float realHeight = heightData_[index];

            aabbMin_.x = std::min(aabbMin_.x, localX);
            aabbMin_.y = std::min(aabbMin_.y, realHeight);
            aabbMin_.z = std::min(aabbMin_.z, localZ);

            aabbMax_.x = std::max(aabbMax_.x, localX);
            aabbMax_.y = std::max(aabbMax_.y, realHeight); 
            aabbMax_.z = std::max(aabbMax_.z, localZ);
        }
    }

    // カメラ接近時のフラスタムカリング誤判定を防ぐため、AABBに余白を持たせる
    float padding = cellSize_ * 2.0f; // 安全圏としてセル2つ分の余白を持たせる

    aabbMin_.x -= padding;
    aabbMin_.y -= padding;
    aabbMin_.z -= padding;

    aabbMax_.x += padding;
    aabbMax_.y += padding;
    aabbMax_.z += padding;

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();

    // 頂点バッファの生成 (初回のみ)
    size_t vbSizeInBytes = sizeof(TerrainVertexData) * vertices.size();
    vertexBuffer_ = BufferManager::CreateBufferResource(device, vbSizeInBytes);
    if (!vertexBuffer_) return false;

    void* mappedPtr = nullptr;
    vertexBuffer_->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, vertices.data(), vbSizeInBytes);
    vertexBuffer_->Unmap(0, nullptr);

    vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = static_cast<UINT>(vbSizeInBytes);
    vbView_.StrideInBytes = sizeof(TerrainVertexData);

    // インデックスバッファの生成 (初回のみ)
    std::vector<uint32_t> indices;
    for (int z = 0; z < numCellsZ_; ++z)
    {
        for (int x = 0; x < numCellsX_; ++x)
        {
            uint32_t i00 = z * numVertsX + x;
            uint32_t i10 = z * numVertsX + (x + 1);
            uint32_t i01 = (z + 1) * numVertsX + x;
            uint32_t i11 = (z + 1) * numVertsX + (x + 1);

            indices.push_back(i00); indices.push_back(i01); indices.push_back(i10);
            indices.push_back(i10); indices.push_back(i01); indices.push_back(i11);
        }
    }
    indexCount_ = static_cast<UINT>(indices.size());
    size_t ibSizeInBytes = sizeof(uint32_t) * indices.size();

    indexBuffer_ = BufferManager::CreateBufferResource(device, ibSizeInBytes);
    if (!indexBuffer_) return false;

    void* ibMappedPtr = nullptr;
    indexBuffer_->Map(0, nullptr, &ibMappedPtr);
    memcpy(ibMappedPtr, indices.data(), ibSizeInBytes);
    indexBuffer_->Unmap(0, nullptr);

    ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = static_cast<UINT>(ibSizeInBytes);
    ibView_.Format = DXGI_FORMAT_R32_UINT;

    return true;
}

void TerrainChunk::SetHeightData(const std::vector<float>& localHeightData)
{
    heightData_ = localHeightData;
}

}