#pragma once
#include "WorldTransform.h"
#include "BlendMode.h" 
#include "RenderCommon.h" 

namespace FE
{

class Engine;

class Model
{
public:
    // コンストラクタ
    Model(Engine* engine, const std::string& modelName);
    // 内部処理用
    Model(Engine* engine, const ModelData* modelData);
    ~Model() = default;

    // 描画処理
    void Draw();

    // UV情報の更新
    void UpdateUV();

    // ========================================================================
    // 基本トランスフォーム
    // ========================================================================
    void SetTransform(const WorldTransform& transform) { transform_ = transform; }
    const WorldTransform& GetTransform() const { return transform_; }
    WorldTransform& GetTransform() { return transform_; }

    // ========================================================================
    // 一括設定 (全マテリアルへ適用)
    // ========================================================================
    // テクスチャ
    void SetTexture(const std::string& textureName);
    void SetEnvironmentMapTexture(const std::string& textureName);
    void SetToonRampTexture(const std::string& textureName);
    void SetDissolveTexture(const std::string& textureName);
    void SetNormalMapTexture(const std::string& textureName);
    void SetRippleTexture(const std::string& textureName);
    void SetPuddleNoiseTexture(const std::string& textureName);

    // 別のモデルからマテリアル情報をすべてコピーする関数
    void CopyMaterialsFrom(const Model* sourceModel);
    void ShareMaterialsFrom(const Model* sourceModel);
    void ShareModelDataFrom(const Model* sourceModel);
    // マテリアルを共有状態から切り離し、自分専用のクローンにする
    void MakeMaterialUnique();

    // UV
    void SetUVTransform(const WorldTransform& uvTransform);

    // カラー・発光
    void SetColor(const Vector4& color);
    void SetColor(uint32_t color);
    void SetEmissiveIntensity(float intensity);
    // マテリアルは触らず、モデル自体の色を変える
    void SetBaseColor(const Vector4& color) { baseColor_ = color; }
    void SetBaseColor(uint32_t color);

    // アウトライン
    void SetEnableOutline(bool enable);
    void SetOutlineWidth(float width);
    void SetOutlineColor(const Vector4& color);
    void SetOutlineColor(uint32_t color);

    // ディゾルブ
    void SetEnableDissolve(bool enable);

    // 描画ステート
    void SetRenderGroup(RenderGroup group) { renderGroup_ = group; }
    void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }
    BlendMode GetBlendMode() const { return blendMode_; }
    void SetCullMode(CullMode mode) { cullMode_ = mode; }
    void SetDepthMode(DepthMode mode) { depthMode_ = mode; }

    // ========================================================================
    // 個別設定
    // ========================================================================
    void SetMaterialColor(size_t index, const Vector4& color);
    void SetMaterialColor(size_t index, uint32_t color);

    // ========================================================================
    // ゲッター
    // ========================================================================

    // UVトランスフォーム取得
    WorldTransform* GetUVTransform(size_t index = 0);

    // マテリアルデータ取得
    MaterialData* GetMaterialData(size_t index = 0);
    const MaterialData* GetMaterialData(size_t index = 0) const;

    // マテリアルハンドル取得
    MaterialHandle* GetMaterialHandle(size_t index = 0);

    // モデル名を取得
    const std::string& GetName() const { return name_; }

    const Vector4& GetBaseColor() const { return baseColor_; }

    // 色ポインタ取得 (ImGui等で直接編集する場合に使用)
    Vector4* GetMaterialColorPtr(size_t index);

    // マテリアル数
    size_t GetMaterialCount() const { return materials_.size(); }

    void ApplyRenderSettings(const RenderSettings& settings);

    const ModelData* GetModelData() const { return modelData_; }

private:
    // ヘルパー関数: 範囲チェック
    bool IsValidMaterialIndex(size_t index) const;

private:
    Engine* engine_ = nullptr;
    const ModelData* modelData_ = nullptr;
    std::string name_;

    // マテリアルとは別に、モデル自体が持つ色
    Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };

    // メッシュごとのマテリアルリスト
    std::vector<MaterialHandle> materials_;
    // 現在のマテリアルが自分専用かどうか
    bool isMaterialsUnique_ = false;

    // モデル自体のトランスフォーム
    WorldTransform transform_;

    // 描画設定
    BlendMode blendMode_ = BlendMode::kBlendModeNone;
    RenderGroup renderGroup_ = RenderGroup::Opaque;
    CullMode cullMode_ = CullMode::Back;
    DepthMode depthMode_ = DepthMode::Write;
};

}