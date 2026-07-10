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

    // 描画処理
    void Draw();

    // パラメータが変わったときにメッシュを再構成する関数
    void RebuildMesh();

    bool LoadFromHeightmap(const std::string& heightmapTexName, int chunkSize, float cellSize);

    // UV情報の更新
    void UpdateUV();

    // ========================================================================
    // 基本トランスフォーム
    // ========================================================================
    void SetTransform(const WorldTransform& transform) { transform_ = transform; }
    const WorldTransform& GetTransform() const { return transform_; }
    WorldTransform& GetTransform() { return transform_; }

    // ========================================================================
    // マテリアル・テクスチャ設定
    // ========================================================================
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
    void SetBaseColor(uint32_t color);

    // アウトライン
    void SetEnableOutline(bool enable);
    void SetOutlineWidth(float width);
    void SetOutlineColor(const Vector4& color);
    void SetOutlineColor(uint32_t color);

    // ディゾルブ
    void SetEnableDissolve(bool enable);

    size_t GetMaterialCount() const { return 1; }

    // ========================================================================
    // ゲッター
    // ========================================================================
    WorldTransform* GetUVTransform();

    MaterialData* GetMaterialData();
    const MaterialData* GetMaterialData() const;

    MaterialHandle* GetMaterialHandle();

    const Vector4& GetBaseColor() const { return baseColor_; }
    Vector4* GetMaterialColorPtr();

    // 高さを取得するヘルパー
    float GetHeightAt(float worldX, float worldZ) const;

    // リアルタイムに調整したいパラメータを構造体として定義
    struct Parameters {
        float maxHeight = 20.0f;
        float uvScale = 0.1f;
       
    };

    Parameters& GetParams() { return params_; }
    const Parameters& GetParams() const { return params_; }

private:
    Engine* engine_ = nullptr;
    std::vector<std::unique_ptr<TerrainChunk>> chunks_;
    MaterialHandle material_;
    WorldTransform transform_;
    Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };

    // === リアルタイム調整のために追加するメンバ変数 ===
    std::string heightmapTexName_;
    int chunkSize_ = 16;
    float cellSize_ = 1.0f;

    // リアルタイムに変動させたいパラメータと、元の比率データを保持
    // インスペクターで直接書き換える変数（デフォルト値を入れておく）
    Parameters params_;

    // ハイトマップ画像から読み込んだ「0.0〜1.0」の純粋な高さデータ（全頂点分）
    // これを保存しておくことで、maxHeight_が変わったときに再計算できます
    std::vector<float> rawHeightRatios_;

    int totalVertsX_ = 0;
    int totalVertsZ_ = 0;
};

}