#pragma once
#include "WorldTransform.h"
#include "BlendMode.h" 
#include "RenderCommon.h" 

namespace FE
{

class Engine;
class TerrainChunk;

class Terrain
{
public:
    Terrain(Engine* engine);
    ~Terrain() = default;

    void Draw();

    // パラメータが変わったときにメッシュを再構成する関数
    void RebuildMesh();

    bool LoadFromHeightmap(const std::string& heightmapTexName, float cellSize);

    // UV情報の更新
    void UpdateUV();

    // 基本トランスフォーム
    void SetTransform(const WorldTransform& transform) { transform_ = transform; }
    const WorldTransform& GetTransform() const { return transform_; }
    WorldTransform& GetTransform() { return transform_; }

    // マテリアル・テクスチャ設定
    void SetTexture(const std::string& textureName);
    void SetEnvironmentMapTexture(const std::string& textureName);
    void SetToonRampTexture(const std::string& textureName);
    void SetDissolveTexture(const std::string& textureName);
    void SetNormalMapTexture(const std::string& textureName);
    void SetRippleTexture(const std::string& textureName);
    void SetPuddleNoiseTexture(const std::string& textureName);

    // UV
    void SetUVTransform(const WorldTransform& uvTransform);

    // カラー・発光
    void SetColor(const Vector4& color);
    void SetColor(uint32_t color);
    void SetEmissiveIntensity(float intensity);

    // マテリアルは触らず、地形自体の色を変える
    void SetBaseColor(const Vector4& color) { baseColor_ = color; }
    const Vector4& GetBaseColor() const { return baseColor_; }
    void SetBaseColor(uint32_t color);

    // アウトライン
    void SetEnableOutline(bool enable);
    void SetOutlineWidth(float width);
    void SetOutlineColor(const Vector4& color);
    void SetOutlineColor(uint32_t color);

    // ディゾルブ
    void SetEnableDissolve(bool enable);

    size_t GetMaterialCount() const { return 1; }

    // ゲッター・セッター
    WorldTransform* GetUVTransform();

    MaterialData* GetMaterialData();
    const MaterialData* GetMaterialData() const;

    MaterialHandle* GetMaterialHandle();
    Vector4* GetMaterialColorPtr();

    // 高さを取得するヘルパー
    float GetHeight(float worldX, float worldZ) const;

    // リアルタイムに調整したいパラメータを構造体として定義
    struct Parameters {
        float maxHeight = 20.0f;
        float uvScale = 0.1f;
        float texelSize = 1.0f / 512.0f;
        float cellSize = 1.0f;
    };

    Parameters& GetParams() { return params_; }
    const Parameters& GetParams() const { return params_; }

    // ゲッター（PropertyBinder用ポインタ拡張）
    std::string* GetHeightmapNamePtr() { return &heightmapTexName_; }
    const std::string& GetHeightmapName() const { return heightmapTexName_; }

    uint32_t* GetHeightmapHandlePtr() { return &heightMapHandle_; }
    uint32_t GetHeightmapHandle() const { return heightMapHandle_; }

    int GetChunkSize() const { return chunkSize_; }

private:
    Engine* engine_ = nullptr;
    std::vector<std::unique_ptr<TerrainChunk>> chunks_;
    MaterialHandle material_;
    WorldTransform transform_; 
    Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };

    // リアルタイム調整用メンバ変数
    std::string heightmapTexName_;
    int chunkSize_ = 16;

    Parameters params_;
    std::vector<float> rawHeightRatios_;

    int totalVertsX_ = 0;
    int totalVertsZ_ = 0;

    uint32_t heightMapHandle_ = 0;
};

}