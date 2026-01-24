#pragma once

#include "AnimationData.h" 
#include "AnimationHandle.h" 
#include "TextureHandle.h"

class Engine;

class AnimationModel
{
public:
    // コンストラクタ
    AnimationModel(Engine* engine, const ModelData* modelData, const Animation* animation);
    ~AnimationModel();

    // 更新処理
    void Update();

    // 描画処理
    void Draw();

    // ========================================================================
    // アニメーション制御
    // ========================================================================

    // アニメーション再生開始
    void Play(const Animation* animation, bool isLoop = true, float speedScale = 1.0f);

    // アニメーションの切り替え (設定は維持)
    void SetAnimation(const Animation* animation);

    // 最初から再生しなおす
    void ResetAnimation();

    // 一時停止 / 再開
    void Stop() { isPlaying_ = false; }
    void Resume() { isPlaying_ = true; }

    // パラメータ変更
    void SetSpeedScale(float speedScale) { speedScale_ = speedScale; }
    void SetIsLoop(bool isLoop) { isLoop_ = isLoop; }
    void SetEasing(EasingType type) { easingType_ = type; }

    // ========================================================================
    // 基本トランスフォーム
    // ========================================================================
    void SetTransform(const WorldTransform& transform) { transform_ = transform; }
    void SetUVTransform(const WorldTransform& uvTransform); // 全マテリアル一括

    // ========================================================================
    // マテリアル一括設定 (全マテリアルへ適用)
    // ========================================================================
    // テクスチャ
    void SetTexture(TextureID textureID);
    void SetEnvironmentMapTexture(TextureID textureID);
    void SetToonRampTexture(TextureID textureID);
    void SetDissolveTexture(TextureID textureID);
    void SetNormalMapTexture(TextureID textureID);

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

    // ========================================================================
    // マテリアル個別設定
    // ========================================================================
    void SetMaterialColor(size_t index, const Vector4& color);
    void SetMaterialColor(size_t index, uint32_t color);

    // ========================================================================
    // ゲッター / アクセサ
    // ========================================================================

    // ステート取得
    bool IsPlaying() const { return isPlaying_; }
    bool IsFinished() const { return isFinished_; }
    bool IsLoop() const { return isLoop_; }
    float GetAnimationTime() const { return animationTime_; }
    float GetSpeedScale() const { return speedScale_; }

    // トランスフォーム
    WorldTransform& GetTransform() { return transform_; }
    const WorldTransform& GetTransform() const { return transform_; }

    // マテリアル関連
    MaterialData* GetMaterialData(size_t index = 0);
    const MaterialData* GetMaterialData(size_t index = 0) const;

    MaterialHandle* GetMaterialHandle(size_t index = 0);
    Vector4* GetMaterialColorPtr(size_t index); // ImGui等での編集用
    size_t GetMaterialCount() const { return materials_.size(); }

    // ImGui用: UV更新処理
    void UpdateUV();

    // ========================================================================
    // Bind用
    // ========================================================================
    float* GetSpeedScalePtr() { return &speedScale_; }
    bool* GetIsLoopPtr() { return &isLoop_; }

private:
    // ヘルパー: インデックス検証
    bool IsValidMaterialIndex(size_t index) const;

private:
    Engine* engine_ = nullptr;

    // マテリアルリスト
    std::vector<MaterialHandle> materials_;

    // モデル・アニメーションデータ
    AnimatedModelData animeModelData_;

    // 姿勢制御
    Skeleton skeleton_;
    SkinCluster skinCluster_;
    WorldTransform transform_;

    // アニメーション制御パラメータ
    float animationTime_ = 0.0f;
    float speedScale_ = 1.0f;
    bool isLoop_ = true;
    bool isPlaying_ = false;
    bool isFinished_ = false;
    EasingType easingType_ = EasingType::EaseLinear;

    // 描画設定
    BlendMode blendMode_ = BlendMode::kBlendModeNone;
    RenderGroup renderGroup_ = RenderGroup::Opaque;
};