#include "pch.h"
#include "Terrain.h"
#include "TextureManager.h"
#include "Engine.h"
#include "GraphicsDevice.h"
#include "BufferManager.h"
#include "Frustum.h"
#include "TerrainRenderer.h"

namespace FS
{

namespace
{

// チャンクの始まりの位置を並べる。最後のチャンクは前のチャンクと重ねて、地形の端に揃える
std::vector<uint32_t> CalculateChunkStarts(uint32_t totalCells, uint32_t chunkCells)
{
    std::vector<uint32_t> starts;
    for (uint32_t start = 0; start < totalCells; start += chunkCells)
    {
        starts.push_back(std::min(start, totalCells - chunkCells));
    }
    return starts;
}

}

Terrain::Terrain(Engine* engine)
    : engine_(engine)
{
    material_.resource = BufferManager::CreateMappedConstantBuffer(engine_->GetGraphicsDevice()->GetDevice(), &material_.data);
    material_.resource->SetName(L"TerrainMaterial");

    TerrainMaterialData& data = *material_.data;
    data = {};
    data.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    data.roughness = 1.0f;
    data.environmentMapIntensity = 0.5f;
    data.emissiveIntensity = 1.0f;
    data.addShadow = 1;
    data.shadowBias = 0.0005f;
    data.shadowDensity = 0.8f;
    data.normalTiling = 1.0f;
    data.normalIntensity = 1.0f;
    data.rippleScale = 0.4f;
    data.rippleSpeed = 0.4f;
    data.rippleStrength = 10.0f;
    data.rippleFrequency = 6.0f;

    auto& textureManager = TextureManager::GetInstance();
    material_.albedoMapHandle = textureManager.Get(material_.albedoMapName);
    material_.environmentMapHandle = textureManager.Get(material_.environmentMapName);
    material_.normalMapHandle = textureManager.Get(material_.normalMapName);
    material_.rippleMapHandle = textureManager.Get(material_.rippleMapName);
}

void Terrain::Draw()
{
    if (chunks_.empty()) return;

    Frustum frustum;
    frustum.ExtractFromMatrix(engine_->GetViewMatrix() * engine_->GetProjectionMatrix());

    const Vector3 origin = GetOrigin();
    const Vector2 chunkSize = { chunkCellsX_ * params_.cellSize, chunkCellsZ_ * params_.cellSize };

    visibleChunks_.clear();
    for (const Chunk& chunk : chunks_)
    {
        const Vector3 boundsMin = {
            origin.x + chunk.startX * params_.cellSize,
            origin.y + (chunk.minHeightRatio - 0.5f) * params_.maxHeight,
            origin.z + chunk.startZ * params_.cellSize };
        const Vector3 boundsMax = {
            boundsMin.x + chunkSize.x,
            origin.y + (chunk.maxHeightRatio - 0.5f) * params_.maxHeight,
            boundsMin.z + chunkSize.y };

        if (frustum.IntersectsAABB(boundsMin, boundsMax))
        {
            visibleChunks_.push_back({ chunk.startX, chunk.startZ });
        }
    }

    // 見えるチャンクがなくても、草や植物の生成が地形の設定を使うので必ず送る
    engine_->GetRendererManager()->SubmitTerrain(CreateDrawData(), visibleChunks_);
}

TerrainDrawData Terrain::CreateDrawData() const
{
    TerrainDrawData drawData;
    drawData.vertexBufferView = vertexBufferView_;
    drawData.indexBufferView = indexBufferView_;
    drawData.indexCount = indexCount_;
    drawData.material = &material_;
    drawData.heightMapHandle = heightMapHandle_;
    drawData.settings.origin = GetOrigin();
    drawData.settings.maxHeight = params_.maxHeight;
    drawData.settings.cellSize = params_.cellSize;
    drawData.settings.texelSize = 1.0f / static_cast<float>(heightMapWidth_);
    drawData.settings.heightMapWidth = heightMapWidth_;
    drawData.settings.heightMapHeight = heightMapHeight_;
    return drawData;
}

bool Terrain::LoadFromHeightmap(const std::string& heightmapTextureName, float cellSize)
{
    const TextureHandleData* metaData = TextureManager::GetInstance().GetMetaData(heightmapTextureName);
    if (!metaData) return false;

    // GPU に送ったものとは別に、高さを調べるために CPU 側でも画像を読む
    DirectX::ScratchImage image = TextureLoader::LoadTexture(metaData->fullPath);
    const DirectX::TexMetadata& imageMetaData = image.GetMetadata();
    const DirectX::Image* pixels = image.GetImage(0, 0, 0);
    if (!pixels || !pixels->pixels || imageMetaData.width < 2 || imageMetaData.height < 2) return false;

    heightmapTextureName_ = heightmapTextureName;
    heightMapHandle_ = metaData->handle;
    heightMapWidth_ = static_cast<uint32_t>(imageMetaData.width);
    heightMapHeight_ = static_cast<uint32_t>(imageMetaData.height);
    params_.cellSize = cellSize;

    // 16bit なら段差が出にくいのでそのまま使い、それ以外は先頭のチャンネルを高さにする
    const bool is16Bit = imageMetaData.format == DXGI_FORMAT_R16_UNORM;
    const size_t bytesPerPixel = DirectX::BitsPerPixel(imageMetaData.format) / 8;

    heightRatios_.resize(heightMapWidth_ * heightMapHeight_);
    for (uint32_t z = 0; z < heightMapHeight_; ++z)
    {
        const uint8_t* row = pixels->pixels + z * pixels->rowPitch;
        for (uint32_t x = 0; x < heightMapWidth_; ++x)
        {
            heightRatios_[z * heightMapWidth_ + x] = is16Bit
                ? reinterpret_cast<const uint16_t*>(row)[x] / 65535.0f
                : row[x * bytesPerPixel] / 255.0f;
        }
    }

    CreateChunks();
    CreateMesh();
    MarkShapeChanged();

    // 最初の Draw より前に草や植物が生成されても、地形の設定が入っているようにする
    engine_->GetRendererManager()->SubmitTerrain(CreateDrawData(), {});
    return true;
}

void Terrain::CreateChunks()
{
    const uint32_t totalCellsX = heightMapWidth_ - 1;
    const uint32_t totalCellsZ = heightMapHeight_ - 1;
    chunkCellsX_ = std::min(kChunkCellCount, totalCellsX);
    chunkCellsZ_ = std::min(kChunkCellCount, totalCellsZ);

    chunks_.clear();
    for (uint32_t startZ : CalculateChunkStarts(totalCellsZ, chunkCellsZ_))
    {
        for (uint32_t startX : CalculateChunkStarts(totalCellsX, chunkCellsX_))
        {
            Chunk chunk;
            chunk.startX = startX;
            chunk.startZ = startZ;
            chunk.minHeightRatio = FLT_MAX;
            chunk.maxHeightRatio = -FLT_MAX;

            for (uint32_t z = startZ; z <= startZ + chunkCellsZ_; ++z)
            {
                for (uint32_t x = startX; x <= startX + chunkCellsX_; ++x)
                {
                    chunk.minHeightRatio = std::min(chunk.minHeightRatio, GetHeightRatio(x, z));
                    chunk.maxHeightRatio = std::max(chunk.maxHeightRatio, GetHeightRatio(x, z));
                }
            }
            chunks_.push_back(chunk);
        }
    }
}

void Terrain::CreateMesh()
{
    // 頂点は位置を持たず、チャンクの中で何番目かだけを持つ。高さと位置はシェーダーで決める
    const uint32_t vertexCountX = chunkCellsX_ + 1;
    const uint32_t vertexCountZ = chunkCellsZ_ + 1;

    std::vector<TerrainVertexData> vertices;
    vertices.reserve(vertexCountX * vertexCountZ);
    for (uint32_t z = 0; z < vertexCountZ; ++z)
    {
        for (uint32_t x = 0; x < vertexCountX; ++x)
        {
            vertices.push_back({ x, z });
        }
    }

    // GetHeightAt と同じく、(x + 1, z) と (x, z + 1) を結ぶ対角線で三角形に分ける
    std::vector<uint32_t> indices;
    indices.reserve(chunkCellsX_ * chunkCellsZ_ * 6);
    for (uint32_t z = 0; z < chunkCellsZ_; ++z)
    {
        for (uint32_t x = 0; x < chunkCellsX_; ++x)
        {
            const uint32_t index00 = z * vertexCountX + x;
            const uint32_t index10 = index00 + 1;
            const uint32_t index01 = index00 + vertexCountX;
            const uint32_t index11 = index01 + 1;
            indices.insert(indices.end(), { index00, index01, index10, index10, index01, index11 });
        }
    }

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();

    TerrainVertexData* mappedVertices = nullptr;
    vertexBuffer_ = BufferManager::CreateMappedBuffer(device, vertices.size(), &mappedVertices);
    std::memcpy(mappedVertices, vertices.data(), sizeof(TerrainVertexData) * vertices.size());
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<uint32_t>(sizeof(TerrainVertexData) * vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(TerrainVertexData);

    uint32_t* mappedIndices = nullptr;
    indexBuffer_ = BufferManager::CreateMappedBuffer(device, indices.size(), &mappedIndices);
    std::memcpy(mappedIndices, indices.data(), sizeof(uint32_t) * indices.size());
    indexBuffer_->Unmap(0, nullptr);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = static_cast<uint32_t>(sizeof(uint32_t) * indices.size());
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexCount_ = static_cast<uint32_t>(indices.size());
}

Vector3 Terrain::GetOrigin() const
{
    const Vector2 size = GetSizeXZ();
    return { position_.x - size.x * 0.5f, position_.y, position_.z - size.y * 0.5f };
}

Vector2 Terrain::GetSizeXZ() const
{
    if (heightMapWidth_ == 0 || heightMapHeight_ == 0) return { 0.0f, 0.0f };
    return { (heightMapWidth_ - 1) * params_.cellSize, (heightMapHeight_ - 1) * params_.cellSize };
}

bool Terrain::GetHeightAt(float worldX, float worldZ, float& outHeight) const
{
    if (heightRatios_.empty()) return false;

    const Vector3 origin = GetOrigin();
    const float gridX = (worldX - origin.x) / params_.cellSize;
    const float gridZ = (worldZ - origin.z) / params_.cellSize;
    if (gridX < 0.0f || gridZ < 0.0f) return false;

    const uint32_t cellX = static_cast<uint32_t>(gridX);
    const uint32_t cellZ = static_cast<uint32_t>(gridZ);
    if (cellX >= heightMapWidth_ - 1 || cellZ >= heightMapHeight_ - 1) return false;

    // メッシュと同じ三角形の上で補間する
    const float u = gridX - cellX;
    const float v = gridZ - cellZ;
    const float height00 = GetHeightRatio(cellX, cellZ);
    const float height10 = GetHeightRatio(cellX + 1, cellZ);
    const float height01 = GetHeightRatio(cellX, cellZ + 1);
    const float height11 = GetHeightRatio(cellX + 1, cellZ + 1);

    const float heightRatio = (u + v <= 1.0f)
        ? height00 + (height10 - height00) * u + (height01 - height00) * v
        : height11 + (height01 - height11) * (1.0f - u) + (height10 - height11) * (1.0f - v);

    outHeight = origin.y + (heightRatio - 0.5f) * params_.maxHeight;
    return true;
}

float Terrain::GetHeight(float worldX, float worldZ) const
{
    float height = 0.0f;
    return GetHeightAt(worldX, worldZ, height) ? height : 0.0f;
}

}
