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
    transform_.UpdateMatrix();

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
        // チャンクのローカルAABBを取得
        Vector3 aabbMin = chunk->GetAABBMin();
        Vector3 aabbMax = chunk->GetAABBMax();

        // transform_ の移動とスケールを適用してワールド座標のAABBに変換
        aabbMin.x = (aabbMin.x * transform_.scale_.x) + transform_.translation_.x;
        aabbMin.y = (aabbMin.y * transform_.scale_.y) + transform_.translation_.y;
        aabbMin.z = (aabbMin.z * transform_.scale_.z) + transform_.translation_.z;

        aabbMax.x = (aabbMax.x * transform_.scale_.x) + transform_.translation_.x;
        aabbMax.y = (aabbMax.y * transform_.scale_.y) + transform_.translation_.y;
        aabbMax.z = (aabbMax.z * transform_.scale_.z) + transform_.translation_.z;

        // ★ ワールド座標に変換したAABBで判定する
        if (frustum.IntersectsAABB(aabbMin, aabbMax))
        {
            engine_->GetRendererManager()->SubmitTerrain(
                transform_,
                chunk.get(),
                material_,
                baseColor_
            );
        }
    }
}

void Terrain::RebuildMesh()
{
    if (rawHeightRatios_.empty()) return;

    // ★ 全体の幅と奥行きを計算し、その半分をオフセットとする
    float totalWidth = (totalVertsX_ - 1) * cellSize_;
    float totalDepth = (totalVertsZ_ - 1) * cellSize_;
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

                // ★ コンストラクタに offsetX, offsetZ を渡す
                chunks_.push_back(std::make_unique<TerrainChunk>(
                    engine_, startX, startZ, numCellsX, numCellsZ, cellSize_, offsetX, offsetZ
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

                // ★ Y軸の中心化
                // 0.0 ~ 1.0 の比率から 0.5 を引いて、-0.5 ~ +0.5 に変換。
                // これにより、中間グレーが高さ0(平地)となり、maxHeightを変えても平地の高さが維持されます。
                float centeredRatio = rawHeightRatios_[globalIndex] - 0.5f;
                localHeights[z * numVertsX + x] = centeredRatio * params_.maxHeight;
            }
        }

        chunk->SetUVScale(params_.uvScale);
        chunk->SetHeightData(localHeights);
        chunk->CreateMesh();
    }
}

bool Terrain::LoadFromHeightmap(const std::string& heightmapTexName, int chunkSize, float cellSize)
{
    // ★ maxHeight は引数から削除し、params_.maxHeight を使います
    auto& texManager = TextureManager::GetInstance();
    const TextureHandleData* meta = texManager.GetMetaData(heightmapTexName);
    if (!meta) return false;

    DirectX::ScratchImage mipImages = TextureLoader::LoadTexture(meta->fullPath);
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
    const DirectX::Image* img = mipImages.GetImage(0, 0, 0);
    if (!img || !img->pixels) return false;

    totalVertsX_ = static_cast<int>(metadata.width);
    totalVertsZ_ = static_cast<int>(metadata.height);
    chunkSize_ = chunkSize;
    cellSize_ = cellSize;

    // 比率データを保存する配列をリサイズ
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

            // ★ ここでは params_.maxHeight を掛けず、純粋な 0.0 ~ 1.0 の比率だけを保存する！
            rawHeightRatios_[index] = normalizedHeight;
        }
    }

    // ★ transform_ をマイナス移動させていた処理はもう不要なので「削除」し、
      // 確実に初期状態(0,0,0)にしておく
    transform_.translation_ = { 0.0f, 0.0f, 0.0f };
    transform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    transform_.scale_ = { 1.0f, 1.0f, 1.0f };
    transform_.UpdateMatrix();

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

float Terrain::GetHeightAt(float worldX, float worldZ) const
{
    if (chunks_.empty()) return 0.0f;

    // 1. ワールド座標を Terrain 全体のローカル座標に変換
    float localX = (worldX - transform_.translation_.x) / transform_.scale_.x;
    float localZ = (worldZ - transform_.translation_.z) / transform_.scale_.z;

    // 2. どのチャンクに乗っているか探す
    for (const auto& chunk : chunks_)
    {
        Vector3 aabbMin = chunk->GetAABBMin();
        Vector3 aabbMax = chunk->GetAABBMax();

        // XとZがこのチャンクの範囲内か判定（Yは高さなので無視）
        if (localX >= aabbMin.x && localX <= aabbMax.x &&
            localZ >= aabbMin.z && localZ <= aabbMax.z)
        {
            // 一致するチャンクが見つかったら高さを計算して返す
            float localHeight = chunk->GetHeightAt(localX, localZ);
            return (localHeight * transform_.scale_.y) + transform_.translation_.y;
        }
    }

    return 0.0f; // どのチャンクの範囲外だった場合
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