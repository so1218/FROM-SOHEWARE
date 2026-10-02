#pragma once
#include "AnimationData.h" 
#include "AnimationManager.h" 
#include "BlendMode.h" 
#include "RenderCommon.h" 
#include "Easing.h" 

namespace FE
{

class Engine;

class AnimationModel
{
public:
    // コンストラクタ
    AnimationModel(Engine* engine, const std::string& modelName, const std::string& animationName);
    // 内部生成用
    AnimationModel(Engine* engine, const ModelData* modelData, const Animation* animation);
    ~AnimationModel();

    // 更新処理
    void Update();

    // 描画処理
    void Draw();

    // ========================================================================
    // アニメーション制御 (文字列)
    // ========================================================================
    // 名前指定で再生
    void Play(const std::string& animationName, bool isLoop = true, float speedScale = 1.0f, float blendTime = 0.2f);
    void Play(const Animation* animation, bool isLoop = true, float speedScale = 1.0f, float blendTime = 0.2f);

    // 名前指定で切り替え
    void SetAnimation(const std::string& animationName);

    // ========================================================================
    // アニメーション制御 (内部処理用)
    // ========================================================================
    void Play(const Animation* animation, bool isLoop = true, float speedScale = 1.0f);
    void SetAnimation(const Animation* animation);

    // 最初から再生しなおす
    void ResetAnimation();

    // 特定のボーンに回転オフセットを加算する
    void AddJointRotationOffset(const std::string& jointName, const Quaternion& offsetRotation);

    // オフセット適用後にスケルトンとスキニング行列を再計算する
    void PostUpdateSkeleton();

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
    void SetTexture(const std::string& textureName);
    void SetEnvironmentMapTexture(const std::string& textureName);
    void SetToonRampTexture(const std::string& textureName);
    void SetDissolveTexture(const std::string& textureName);
    void SetNormalMapTexture(const std::string& textureName);
    void SetRippleTexture(const std::string& textureName);
    void SetPuddleNoiseTexture(const std::string& textureName);

    // カラー・発光
    void SetColor(const Vector4& color);
    void SetColor(uint32_t color);
    void SetBaseColor(const Vector4& color) { baseColor_ = color; }
    void SetBaseColor(uint32_t color);
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

    // スケルトン取得
    const Skeleton& GetSkeleton() const { return skeleton_; }

    // 指定した名前のボーンのワールド行列を取得
    Matrix4x4 GetJointWorldMatrix(const std::string& jointName) const;

    // マテリアル関連
    MaterialData* GetMaterialData(size_t index = 0);
    const MaterialData* GetMaterialData(size_t index = 0) const;

    MaterialHandle* GetMaterialHandle(size_t index = 0);
    Vector4* GetMaterialColorPtr(size_t index); // ImGui等での編集用
    size_t GetMaterialCount() const { return materials_.size(); }

    const Vector4& GetBaseColor() const { return baseColor_; }

    // ImGui用: UV更新処理
    void UpdateUV();

    // ========================================================================
    // Bind用
    // ========================================================================
    float* GetSpeedScalePtr() { return &speedScale_; }
    bool* GetIsLoopPtr() { return &isLoop_; }

    const ModelData* GetModelData() const { return animeModelData_.modelData; }

    // スキニング後の頂点座標を取得する関数
    Vector3 GetSkinnedVertexPosition(size_t meshIndex, size_t vertexIndex) const;

private:
    // ヘルパー: インデックス検証
    bool IsValidMaterialIndex(size_t index) const;

private:
    Engine* engine_ = nullptr;

    // マテリアルリスト
    std::vector<MaterialHandle> materials_;

    // モデル・アニメーションデータ
    AnimatedModelData animeModelData_;

    // モデル全体の色
    Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };

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

    const Animation* prevAnimation_ = nullptr; // 遷移前のアニメーション
    float prevAnimationTime_ = 0.0f;           // 遷移前のアニメーション時間
    float blendDuration_ = 0.0f;               // ブレンドにかける総時間
    float blendTimer_ = 0.0f;                  // ブレンド経過時間
    bool isBlending_ = false;                  // ブレンド中フラグ

    // 描画設定
    BlendMode blendMode_ = BlendMode::kBlendModeNone;
    RenderGroup renderGroup_ = RenderGroup::Opaque;
};

}