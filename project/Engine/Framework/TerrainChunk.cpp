#include "pch.h"
#include "TerrainChunk.h"
#include "TextureManager.h"
#include "TextureLoader.h"
#include "Engine.h"
#include "BufferManager.h"

namespace FE
{

TerrainChunk::TerrainChunk(Engine* engine, int startX, int startZ, int numCellsX, int numCellsZ, float cellSize, float offsetX, float offsetZ)
    : engine_(engine), startX_(startX), startZ_(startZ), numCellsX_(numCellsX), numCellsZ_(numCellsZ), cellSize_(cellSize), offsetX_(offsetX), offsetZ_(offsetZ)
{
    int numVertsX = numCellsX_ + 1;
    int numVertsZ = numCellsZ_ + 1;
    heightData_.resize(numVertsX * numVertsZ, 0.0f);
}

float TerrainChunk::GetHeightAt(float targetX, float targetZ) const
{
    // ★ チャンクの開始位置（オフセット）を差し引いて、チャンク内ローカル座標にする
    float chunkLocalX = (targetX + offsetX_) - (startX_ * cellSize_);
    float chunkLocalZ = (targetZ + offsetZ_) - (startZ_ * cellSize_);

    // 1. グリッドのインデックス用に変換
    float localX = chunkLocalX / cellSize_;
    float localZ = chunkLocalZ / cellSize_;

    // 2. どのマス目（セル）にいるか、インデックスを割り出す
    int cellX = static_cast<int>(std::floor(localX));
    int cellZ = static_cast<int>(std::floor(localZ));

    // 範囲外なら0を返す
    if (cellX < 0 || cellX >= numCellsX_ || cellZ < 0 || cellZ >= numCellsZ_) {
        return 0.0f;
    }

    // 3. マス目の中での小数部分（0.0 ～ 1.0）を取得
    float u = localX - cellX;
    float v = localZ - cellZ;

    // 4. マス目を構成する4つの頂点の高さを取得
    int numVertsX = numCellsX_ + 1;
    float h00 = heightData_[cellZ * numVertsX + cellX];         // 左上 (A)
    float h10 = heightData_[cellZ * numVertsX + (cellX + 1)];   // 右上 (B)
    float h01 = heightData_[(cellZ + 1) * numVertsX + cellX];     // 左下 (C)
    float h11 = heightData_[(cellZ + 1) * numVertsX + (cellX + 1)]; // 右下 (D)

    // 5. 三角形のどちらに属しているかで高さを補間計算
    // マス目は左上から右下へ対角線で割られていると仮定します。
    if (u + v <= 1.0f)
    {
        // 左上の三角形 (A, B, C) 上にいる場合
        // 高さは A を基準に、X方向の傾きとZ方向の傾きを足し合わせる
        return h00 + (h10 - h00) * u + (h01 - h00) * v;
    }
    else
    {
        // 右下の三角形 (D, B, C) 上にいる場合
        // 高さは D (右下) を基準に、逆方向から計算
        return h11 + (h01 - h11) * (1.0f - u) + (h10 - h11) * (1.0f - v);
    }
}

bool TerrainChunk::CreateMesh()
{
    int numVertsX = numCellsX_ + 1;
    int numVertsZ = numCellsZ_ + 1;
    size_t vertexCount = numVertsX * numVertsZ;

    std::vector<VertexData> vertices(vertexCount);

    aabbMin_ = { 999999.0f, 999999.0f, 999999.0f };
    aabbMax_ = { -999999.0f, -999999.0f, -999999.0f };

    for (int z = 0; z < numVertsZ; ++z)
    {
        for (int x = 0; x < numVertsX; ++x)
        {
            int index = z * numVertsX + x;

            // ★ 頂点を生成する際、全体幅の半分（offsetX_ / offsetZ_）を引いて中心を0にする
            float localX = (startX_ + x) * cellSize_ - offsetX_;
            float localZ = (startZ_ + z) * cellSize_ - offsetZ_;

            // 高さはTerrain側ですでに中心化されたものが heightData_ に入っている
            float localY = heightData_[index];

            vertices[index].position.x = localX;
            vertices[index].position.y = localY;
            vertices[index].position.z = localZ;
            vertices[index].position.w = 1.0f;

            aabbMin_.x = std::min(aabbMin_.x, localX);
            aabbMin_.y = std::min(aabbMin_.y, localY);
            aabbMin_.z = std::min(aabbMin_.z, localZ);

            aabbMax_.x = std::max(aabbMax_.x, localX);
            aabbMax_.y = std::max(aabbMax_.y, localY);
            aabbMax_.z = std::max(aabbMax_.z, localZ);

            // ハードコードされていた0.1fを変数に変更
            vertices[index].texcoord.x = (startX_ + x) * uvScale_;
            vertices[index].texcoord.y = (startZ_ + z) * uvScale_;
        }
    }

    ComputeNormals(vertices, numVertsX, numVertsZ);

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    size_t vbSizeInBytes = sizeof(VertexData) * vertices.size();

    // ★ 修正点1: 頂点バッファが未生成の場合のみ新規作成
    if (!vertexBuffer_)
    {
        vertexBuffer_ = BufferManager::CreateBufferResource(device, vbSizeInBytes);
        if (!vertexBuffer_) return false;

        vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
        vbView_.SizeInBytes = static_cast<UINT>(vbSizeInBytes);
        vbView_.StrideInBytes = sizeof(VertexData);
    }

    // データのコピー（毎フレームここだけが走るため爆速）
    void* mappedPtr = nullptr;
    vertexBuffer_->Map(0, nullptr, &mappedPtr);
    memcpy(mappedPtr, vertices.data(), vbSizeInBytes);
    vertexBuffer_->Unmap(0, nullptr);


    // ★ 修正点2: インデックスデータはトポロジー（繋がり）が変わらないので、初回のみ生成
    if (!indexBuffer_)
    {
        std::vector<uint32_t> indices;
        for (int z = 0; z < numCellsZ_; ++z)
        {
            for (int x = 0; x < numCellsX_; ++x)
            {
                uint32_t i00 = z * numVertsX + x;
                uint32_t i10 = z * numVertsX + (x + 1);
                uint32_t i01 = (z + 1) * numVertsX + x;
                uint32_t i11 = (z + 1) * numVertsX + (x + 1);

                // 三角形1 (左上、左下、右上の順)
                indices.push_back(i00);
                indices.push_back(i01); 
                indices.push_back(i10); 

                // 三角形2 (右上、左下、右下の順)
                indices.push_back(i10);
                indices.push_back(i01); 
                indices.push_back(i11);
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
    }

    return true;
}

void TerrainChunk::SetHeightData(const std::vector<float>& localHeightData)
{
    heightData_ = localHeightData;
}

// 隣り合う頂点との高低差から、傾き（法線ベクトル）を計算して滑らかな陰影を作るロジック
void TerrainChunk::ComputeNormals(std::vector<VertexData>&vertices, int numVertsX, int numVertsZ)
{
    for (int z = 0; z < numVertsZ; ++z)
    {
        for (int x = 0; x < numVertsX; ++x)
        {
            int index = z * numVertsX + x;

            // 周辺の頂点の高さを取得（端っこの場合は自分自身の高さを使う）
            float hL = (x > 0) ? vertices[z * numVertsX + (x - 1)].position.y : vertices[index].position.y; // 左
            float hR = (x < numVertsX - 1) ? vertices[z * numVertsX + (x + 1)].position.y : vertices[index].position.y; // 右
            float hD = (z > 0) ? vertices[(z - 1) * numVertsX + x].position.y : vertices[index].position.y; // 下
            float hU = (z < numVertsZ - 1) ? vertices[(z + 1) * numVertsX + x].position.y : vertices[index].position.y; // 上

            // --- 1. 法線 (Normal) の計算 ---
            float nx = hL - hR;
            float ny = 2.0f * cellSize_;
            float nz = hD - hU;

            float nLength = std::sqrt(nx * nx + ny * ny + nz * nz);
            vertices[index].normal.x = nx / nLength;
            vertices[index].normal.y = ny / nLength;
            vertices[index].normal.z = nz / nLength;

            // --- 2. タンジェント (Tangent) の計算 ---
            // 接線（Tangent）は、X方向への移動に伴う「位置の変化（傾き）」を表すベクトルです。
            // Xが1セル進む（2 * cellSize_ 分）ときの、高さの変化（hR - hL）を計算します。
            float tx = 2.0f * cellSize_;
            float ty = hR - hL;
            float tz = 0.0f; // グリッドのX方向への進展なので、Z成分は0

            float tLength = std::sqrt(tx * tx + ty * ty + tz * tz);
            vertices[index].tangent.x = tx / tLength;
            vertices[index].tangent.y = ty / tLength;
            vertices[index].tangent.z = tz / tLength;
        }
    }
}

}