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

    // Transform の初期化
    transform_.translation_ = { 0.0f, 0.0f, 0.0f };
    transform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    transform_.scale_ = { 1.0f, 1.0f, 1.0f };
    transform_.UpdateMatrix();

    // 描画用マテリアルの構築とテクスチャ割り当て
    material_ = engine_->GetMaterialManager()->CreateMaterial(engine_->GetGraphicsDevice()->GetDevice());

    auto& texManager = TextureManager::GetInstance();

    material_.textureHandle = texManager.Get("white1x1");
    material_.envMapHandle = texManager.Get("pureSky");
    material_.toonRampHandle = texManager.Get("toonRamp_01");
    material_.dissolveMapHandle = texManager.Get("white1x1");
    material_.normalMapHandle = texManager.Get("white1x1");

    // テクスチャ座標の変換行列を初期化し、定数バッファへ反映
    material_.uvTransformData.Initialize();
    if (material_.materialData)
    {
        material_.materialData->uvTransform = material_.uvTransformData.matWorld_;
    }
}

void Terrain::Draw()
{
    if (chunks_.empty() || !engine_) return;

    const Matrix4x4& view = engine_->GetViewMatrix();
    const Matrix4x4& proj = engine_->GetProjectionMatrix();
    FE::Frustum frustum;
    frustum.ExtractFromMatrix(view * proj);

    // VRAM消費を抑えるため、全チャンクで共通の頂点バッファ（代表メッシュ）を使い回す
    TerrainChunk* sharedMesh = chunks_[0].get();

    // 地形全体を原点中心に配置するためのオフセット値
    float offsetX = (totalVertsX_ - 1) * params_.cellSize * 0.5f;
    float offsetZ = (totalVertsZ_ - 1) * params_.cellSize * 0.5f;

    for (const auto& chunk : chunks_)
    {
        // 各チャンクのワールド座標を算出し、個別の Transform を作成
        WorldTransform chunkTransform;
        chunkTransform.translation_ = {
            (chunk->GetStartX() * params_.cellSize - offsetX) + transform_.translation_.x, 
            transform_.translation_.y,
            (chunk->GetStartZ() * params_.cellSize - offsetZ) + transform_.translation_.z  
        };
        chunkTransform.UpdateMatrix();

        // チャンクのUVトランスフォームを計算
        float totalVertsX_f = static_cast<float>(totalVertsX_);
        float totalVertsZ_f = static_cast<float>(totalVertsZ_);

        // 頂点シェーダーでのハイトマップ参照用スケール
        float uvScaleX = static_cast<float>(chunk->GetNumCellsX()) / totalVertsX_f;
        float uvScaleZ = static_cast<float>(chunk->GetNumCellsZ()) / totalVertsZ_f;

        // ハイトマップサンプリング時のテクセルずれを防ぐため、半ピクセル分のオフセットを加算
        float uvOffsetX = (static_cast<float>(chunk->GetStartX()) + 0.5f) / totalVertsX_f;
        float uvOffsetZ = (static_cast<float>(chunk->GetStartZ()) + 0.5f) / totalVertsZ_f;
        Vector4 uvTransform = { uvScaleX, uvScaleZ, uvOffsetX, uvOffsetZ };

        // 視界判定用の AABB をワールド空間へ変換
        // 高さはハイトマップの最大適用値を乗算してスケールを合わせる
        Vector3 aabbMin = chunk->GetAABBMin();
        Vector3 aabbMax = chunk->GetAABBMax();
        Vector3 worldMin = {
              aabbMin.x + chunkTransform.translation_.x,
              (aabbMin.y * params_.maxHeight) + transform_.translation_.y, 
              aabbMin.z + chunkTransform.translation_.z
        };
        Vector3 worldMax = {
            aabbMax.x + chunkTransform.translation_.x,
            (aabbMax.y * params_.maxHeight) + transform_.translation_.y, 
            aabbMax.z + chunkTransform.translation_.z
        };

        // カメラの視界内に入るチャンクのみを描画キューに登録
        if (frustum.IntersectsAABB(worldMin, worldMax))
        {
            engine_->GetRendererManager()->SubmitTerrain(
                chunkTransform, 
                sharedMesh,     
                uvTransform,    
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

    float totalWidth = (totalVertsX_ - 1) * params_.cellSize;
    float totalDepth = (totalVertsZ_ - 1) * params_.cellSize;
    float offsetX = totalWidth * 0.5f;
    float offsetZ = totalDepth * 0.5f;

    // 初回構築時のみチャンク分割を実施
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

                chunks_.push_back(std::make_unique<TerrainChunk>(
                    engine_, startX, startZ, numCellsX, numCellsZ, params_.cellSize, offsetX, offsetZ,
                    static_cast<float>(totalVertsX_), static_cast<float>(totalVertsZ_)
                ));
            }
        }
    }

    // 各チャンクにローカルの高さデータを割り当て、AABB を更新
    for (size_t i = 0; i < chunks_.size(); ++i)
    {
        auto& chunk = chunks_[i];

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

                // 地形の基準となる高さをゼロにするための中心化補正
                float ratio = rawHeightRatios_[globalIndex] - 0.5f;
                localHeights[z * numVertsX + x] = ratio;
            }
        }

        chunk->SetUVScale(params_.uvScale);
        chunk->SetHeightData(localHeights);

        // カリング用途の AABB は全チャンクで計算するが、
        // 頂点バッファの実体化は共通メッシュとなる先頭チャンクのみ行う
        chunk->CalculateAABB();
        if (i == 0)
        {
            chunk->CreateMesh();
        }
    }
}

bool Terrain::LoadFromHeightmap(const std::string& heightmapTexName, float cellSize)
{
    auto& texManager = TextureManager::GetInstance();
    const TextureHandleData* meta = texManager.GetMetaData(heightmapTexName);
    if (!meta) return false;

    // ハイトマップ画像をCPU側のメモリ領域へ展開し、直接ピクセルデータへアクセス
    DirectX::ScratchImage mipImages = TextureLoader::LoadTexture(meta->fullPath);
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
    const DirectX::Image* img = mipImages.GetImage(0, 0, 0);
    if (!img || !img->pixels) return false;

    // 画像サイズから地形の頂点解像度を決定し、サンプリング用のテクセルサイズを算出
    totalVertsX_ = static_cast<int>(metadata.width);
    totalVertsZ_ = static_cast<int>(metadata.height);
    params_.texelSize = 1.0f / static_cast<float>(totalVertsX_);

    heightMapHandle_ = TextureManager::GetInstance().Get(heightmapTexName);

    rawHeightRatios_.resize(totalVertsX_ * totalVertsZ_);
    size_t pixelSize = DirectX::BitsPerPixel(metadata.format) / 8;

    // 全ピクセルを走査し、0.0〜1.0の範囲に正規化した高さ情報を抽出
    for (int z = 0; z < totalVertsZ_; ++z) 
    {
        for (int x = 0; x < totalVertsX_; ++x)
        {
            int index = z * totalVertsX_ + x;
            float normalizedHeight = 0.0f;

            // 地形特有の段差を回避するため、16bitグレースケール精度の読み込みに対応
            if (metadata.format == DXGI_FORMAT_R16_UNORM) 
            {
                const uint16_t* srcPixels = reinterpret_cast<const uint16_t*>(img->pixels);
                normalizedHeight = static_cast<float>(srcPixels[index]) / 65535.0f;
            }
            else 
            {
                // 8bitフォーマットの場合は赤チャンネルの値を高さとして採用
                // メモリのアライメント（行パディング）によるズレを防ぐため、rowPitchを用いて正確なアドレスを計算
                const uint8_t* pixelPtr = img->pixels + (z * img->rowPitch) + (x * pixelSize);
                normalizedHeight = static_cast<float>(pixelPtr[0]) / 255.0f;
            }

            rawHeightRatios_[index] = normalizedHeight;
        }
    }

    // 更新された高さ情報に基づいて、地形チャンクとカリング用 AABB を再構築
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
    float height = 0.0f;
    if (GetHeightAt(worldX, worldZ, height))
    {
        return height;
    }
    return 0.0f;
}

bool Terrain::GetHeightAt(float worldX, float worldZ, float& outHeight) const
{
    // ワールド座標から地形ローカル座標へ変換
    float localX = worldX - transform_.translation_.x;
    float localZ = worldZ - transform_.translation_.z;
    float heightRatio = 0.0f;

    for (const auto& chunk : chunks_)
    {
        // チャンクのアタリ判定
        if (chunk->GetHeightAt(localX, localZ, heightRatio))
        {
            outHeight = (heightRatio * params_.maxHeight) + transform_.translation_.y;
            return true; // 地形内であれば true
        }
    }

    return false; // 範囲外であれば false
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