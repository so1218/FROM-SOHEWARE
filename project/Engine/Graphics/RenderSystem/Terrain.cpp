#include "pch.h"
#include "Terrain.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Engine.h"
#include "TerrainChunk.h"
#include "Frustum.h"

namespace FE
{

Terrain::Terrain(Engine* engine)
    : engine_(engine)
{
    if (!engine_) return;

    // ★ 追加: transform_ を初期状態(Identity)で確定させておく
    transform_.translation_ = { 0.0f, 0.0f, 0.0f };
    transform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    transform_.scale_ = { 1.0f, 1.0f, 1.0f };
    transform_.UpdateMatrix();

    // マテリアルの作成
    material_ = engine_->GetMaterialManager()->CreateMaterial(engine_->GetGraphicsDevice()->GetDevice());

    auto& texManager = TextureManager::GetInstance();

    material_.textureHandle = texManager.Get("white1x1");
    material_.envMapHandle = texManager.Get("skybox");
    material_.toonRampHandle = texManager.Get("toonRamp_01");
    material_.dissolveMapHandle = texManager.Get("white1x1");
    material_.normalMapHandle = texManager.Get("white1x1");

    // UVトランスフォームの初期化と定数バッファへの反映
    material_.uvTransformData.Initialize();
    if (material_.materialData)
    {
        material_.materialData->uvTransform = material_.uvTransformData.matWorld_;
    }
}

void Terrain::Draw()
{
    if (chunks_.empty() || !engine_) return;

    // ★ 排除: 固定値なので毎フレームの行列更新は不要
    // transform_.UpdateMatrix(); 

    // ViewとProjectionを取得
    const Matrix4x4& view = engine_->GetViewMatrix();
    const Matrix4x4& proj = engine_->GetProjectionMatrix();
    Matrix4x4 vp = view * proj;

    // フラスタムの構築
    FE::Frustum frustum;
    frustum.ExtractFromMatrix(vp);

    // チャンクごとに視界判定を行い、見えているものだけ描画キューに送る
    for (const auto& chunk : chunks_)
    {
        // チャンクのローカルAABBを取得（すでに原点中心に配置されている）
        Vector3 aabbMin = chunk->GetAABBMin();
        Vector3 aabbMax = chunk->GetAABBMax();

        // ワールド座標用の Min / Max を用意
        Vector3 worldMin, worldMax;

        // ★ 修正・排除: transform_ は固定（位置0, スケール1）なので、
        // スケール乗算や translation の足し算はすべて不要になりました。
        worldMin.x = aabbMin.x;
        worldMax.x = aabbMax.x;
        worldMin.z = aabbMin.z;
        worldMax.z = aabbMax.z;

        // ★ 修正: 地形は原点(0,0,0)基準で中心化されているため、
        // Y軸の範囲は単純に [-maxHeight, +maxHeight] で固定されます。
        worldMin.y = -params_.maxHeight;
        worldMax.y = params_.maxHeight;

        // ワールド座標（＝ローカル座標）に変換したAABBで判定する
        if (frustum.IntersectsAABB(worldMin, worldMax))
        {
            engine_->GetRendererManager()->SubmitTerrain(
                transform_,         // 中身は初期値（Identity）のまま渡す
                chunk.get(),
                material_,
                baseColor_,
                params_,
                heightMapHandle_
            );
        }
    }
}

void Terrain::RebuildMesh()
{
    if (rawHeightRatios_.empty()) return;

    // ★ cellSize_ ではなく params_.cellSize を使うように修正！
    float totalWidth = (totalVertsX_ - 1) * params_.cellSize;
    float totalDepth = (totalVertsZ_ - 1) * params_.cellSize;
    float offsetX = totalWidth * 0.5f;
    float offsetZ = totalDepth * 0.5f;

    if (chunks_.empty())
    {
        int totalCellsX = totalVertsX_ - 1;
        int totalCellsZ = totalVertsZ_ - 1;

        int numChunksX = (totalCellsX + chunkSize_ - 1) / chunkSize_;
        int numChunksZ = (totalCellsZ + chunkSize_ - 1) / chunkSize_;

        for (int cz = 0; cz < numChunksZ; ++cz)
        {
            for (int cx = 0; cx < numChunksX; ++cx)
            {
                int startX = cx * chunkSize_;
                int startZ = cz * chunkSize_;

                int numCellsX = std::min(chunkSize_, totalCellsX - startX);
                int numCellsZ = std::min(chunkSize_, totalCellsZ - startZ);

                // ★ params_.cellSize を渡す
                chunks_.push_back(std::make_unique<TerrainChunk>(
                    engine_, startX, startZ, numCellsX, numCellsZ, params_.cellSize, offsetX, offsetZ,
                    (float)totalVertsX_, (float)totalVertsZ_
                ));
            }
        }
    }

    for (auto& chunk : chunks_)
    {
        int numVertsX = chunk->GetNumCellsX() + 1;
        int numVertsZ = chunk->GetNumCellsZ() + 1;

        std::vector<float> localHeights(numVertsX * numVertsZ);

        for (int z = 0; z < numVertsZ; ++z)
        {
            for (int x = 0; x < numVertsX; ++x)
            {
                int globalX = std::min(chunk->GetStartX() + x, totalVertsX_ - 1);
                int globalZ = std::min(chunk->GetStartZ() + z, totalVertsZ_ - 1);
                int globalIndex = globalZ * totalVertsX_ + globalX;

                // Y軸の中心化
                float ratio = rawHeightRatios_[globalIndex] - 0.5f;
                localHeights[z * numVertsX + x] = ratio;
            }
        }

        chunk->SetUVScale(params_.uvScale);
        chunk->SetHeightData(localHeights);
        chunk->CreateMesh();
    }
}

bool Terrain::LoadFromHeightmap(const std::string& heightmapTexName, float cellSize)
{
    auto& texManager = TextureManager::GetInstance();
    const TextureHandleData* meta = texManager.GetMetaData(heightmapTexName);
    if (!meta) return false;

    DirectX::ScratchImage mipImages = TextureLoader::LoadTexture(meta->fullPath);
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
    const DirectX::Image* img = mipImages.GetImage(0, 0, 0);
    if (!img || !img->pixels) return false;

    totalVertsX_ = static_cast<int>(metadata.width);
    totalVertsZ_ = static_cast<int>(metadata.height);

    params_.texelSize = 1.0f / static_cast<float>(totalVertsX_);

    heightMapHandle_ = TextureManager::GetInstance().Get(heightmapTexName);

    rawHeightRatios_.resize(totalVertsX_ * totalVertsZ_);
    size_t pixelSize = DirectX::BitsPerPixel(metadata.format) / 8;

    for (int z = 0; z < totalVertsZ_; ++z) {
        for (int x = 0; x < totalVertsX_; ++x) {
            int index = z * totalVertsX_ + x;
            float normalizedHeight = 0.0f;

            if (metadata.format == DXGI_FORMAT_R16_UNORM) {
                const uint16_t* srcPixels = reinterpret_cast<const uint16_t*>(img->pixels);
                normalizedHeight = static_cast<float>(srcPixels[index]) / 65535.0f;
            }
            else {
                const uint8_t* pixelPtr = img->pixels + (z * img->rowPitch) + (x * pixelSize);
                normalizedHeight = static_cast<float>(pixelPtr[0]) / 255.0f;
            }

            rawHeightRatios_[index] = normalizedHeight;
        }
    }

    // ★ 排除: コンストラクタで初期化されていれば、ここで毎回リセット・行列更新する必要はありません
    // transform_.translation_ = { 0.0f, 0.0f, 0.0f };
    // transform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    // transform_.scale_ = { 1.0f, 1.0f, 1.0f };
    // transform_.UpdateMatrix();

    chunks_.clear();
    RebuildMesh();

    return true;
}

void Terrain::UpdateUV()
{
    material_.uvTransformData.UpdateMatrix();
    if (material_.materialData)
    {
        material_.materialData->uvTransform = material_.uvTransformData.matWorld_;
    }
}

void Terrain::SetTexture(const std::string& textureName)
{
    material_.textureHandle = TextureManager::GetInstance().Get(textureName);
}
void Terrain::SetEnvironmentMapTexture(const std::string& textureName)
{
    material_.envMapHandle = TextureManager::GetInstance().Get(textureName);
}
void Terrain::SetToonRampTexture(const std::string& textureName)
{
    material_.toonRampHandle = TextureManager::GetInstance().Get(textureName);
}
void Terrain::SetDissolveTexture(const std::string& textureName)
{
    material_.dissolveMapHandle = TextureManager::GetInstance().Get(textureName);
}
void Terrain::SetNormalMapTexture(const std::string& textureName)
{
    material_.normalMapHandle = TextureManager::GetInstance().Get(textureName);
}
void Terrain::SetRippleTexture(const std::string& textureName)
{
    material_.rippleTextureHandle = TextureManager::GetInstance().Get(textureName);
}
void Terrain::SetPuddleNoiseTexture(const std::string& textureName)
{
    material_.puddleNoiseHandle = TextureManager::GetInstance().Get(textureName);
}

void Terrain::SetUVTransform(const WorldTransform& uvTransform)
{
    material_.uvTransformData.translation_ = uvTransform.translation_;
    material_.uvTransformData.rotation_ = uvTransform.rotation_;
    material_.uvTransformData.scale_ = uvTransform.scale_;

    material_.uvTransformData.UpdateMatrix();
    if (material_.materialData) {
        material_.materialData->uvTransform = material_.uvTransformData.matWorld_;
    }
}

void Terrain::SetColor(const Vector4& color)
{
    if (material_.materialData) material_.materialData->color = color;
}
void Terrain::SetColor(uint32_t color)
{
    SetColor(Math::Uint32ToColorVector(color));
}

void Terrain::SetBaseColor(uint32_t color)
{
    baseColor_ = Math::Uint32ToColorVector(color);
}

void Terrain::SetEmissiveIntensity(float intensity)
{
    if (material_.materialData) material_.materialData->emissiveIntensity = intensity;
}

WorldTransform* Terrain::GetUVTransform()
{
    return &material_.uvTransformData;
}

MaterialData* Terrain::GetMaterialData()
{
    return material_.materialData;
}

const MaterialData* Terrain::GetMaterialData() const
{
    return material_.materialData;
}

MaterialHandle* Terrain::GetMaterialHandle()
{
    return &material_;
}

Vector4* Terrain::GetMaterialColorPtr()
{
    if (material_.materialData) {
        return &material_.materialData->color;
    }
    return nullptr;
}

float Terrain::GetHeight(float worldX, float worldZ) const
{
    float heightRatio = 0.0f;
    for (const auto& chunk : chunks_)
    {
        // チャンクからは -0.5 ~ 0.5 の比率が返ってくる
        if (chunk->GetHeightAt(worldX, worldZ, heightRatio))
        {
            // ここで最新の maxHeight を掛ける
            return heightRatio * params_.maxHeight;
        }
    }
    return 0.0f;
}

void Terrain::SetEnableOutline(bool enable)
{
    int flag = enable ? 1 : 0;
    if (material_.materialData) material_.materialData->enableOutline = flag;
}

void Terrain::SetOutlineWidth(float width)
{
    if (material_.materialData) material_.materialData->outlineWidth = width;
}

void Terrain::SetOutlineColor(const Vector4& color)
{
    if (material_.materialData) material_.materialData->outlineColor = color;
}

void Terrain::SetOutlineColor(uint32_t color)
{
    SetOutlineColor(Math::Uint32ToColorVector(color));
}

void Terrain::SetEnableDissolve(bool enable)
{
    int flag = enable ? 1 : 0; 
    if (material_.materialData) material_.materialData->enableDissolve = flag;
}

}