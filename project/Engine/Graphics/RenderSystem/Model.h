#pragma once

#include <d3d12.h>
#include <string>

#include "WorldTransform.h"
#include "TextureHandle.h"

class Engine;

class Model
{
public:
    // コンストラクタ
    Model(Engine* engine, const ModelData* modelData);
    ~Model() = default;

    // 描画処理
    void Draw();

    // UV情報の更新
    void UpdateUV();

    // ========================================================================
    // 基本トランスフォーム
    // ========================================================================
    void SetWorldTransform(const WorldTransform& transform) { transform_ = transform; }
    const WorldTransform& GetTransform() const { return transform_; }
    WorldTransform& GetTransform() { return transform_; }

    // ========================================================================
    // 一括設定 (全マテリアルへ適用)
    // ========================================================================
    // テクスチャ
    void SetTexture(TextureID textureID);
    void SetEnvironmentMapTexture(TextureID textureID);
    void SetToonRampTexture(TextureID textureID);
    void SetDissolveTexture(TextureID textureID);
    void SetNormalMapTexture(TextureID textureID);

    // UV
    void SetUVTransform(const WorldTransform& uvTransform);

    // カラー・発光
    void SetColor(const Vector4& color);
    void SetColor(uint32_t color);
    void SetEmissiveIntensity(float intensity);

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

    // 色ポインタ取得 (ImGui等で直接編集する場合に使用)
    Vector4* GetMaterialColorPtr(size_t index);

    // マテリアル数
    size_t GetMaterialCount() const { return materials_.size(); }

private:
    // ヘルパー関数: 範囲チェック
    bool IsValidMaterialIndex(size_t index) const;

private:
    Engine* engine_ = nullptr;
    const ModelData* modelData_ = nullptr;

    // メッシュごとのマテリアルリスト
    std::vector<MaterialHandle> materials_;

    // モデル自体のトランスフォーム
    WorldTransform transform_;

    // 描画設定
    BlendMode blendMode_ = BlendMode::kBlendModeNone;
    RenderGroup renderGroup_ = RenderGroup::Opaque;
};