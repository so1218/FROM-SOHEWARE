#pragma once

#include "AnimationData.h" 
#include "AnimationHandle.h" 
#include "TextureHandle.h"

class Engine;

class AnimationModel
{
public:
    AnimationModel(Engine* engine, const ModelData* modelData, const Animation* animation);
    ~AnimationModel();

    void Update();
    void Play(const Animation* animation, bool isLoop = true, float speedScale = 1.0f);
    void Draw();

    // アニメーション制御 
    void ResetAnimation();
    void SetAnimation(const Animation* animation);

    // 途中変更用のセッター
    void SetSpeedScale(float speedScale) { speedScale_ = speedScale; }
    void SetIsLoop(bool isLoop) { isLoop_ = isLoop; }

    // 複雑なセッター
    // テクスチャ関係
    void SetTexture(TextureID textureID);
    void SetEnvironmentMapTexture(TextureID textureID);
    void SetToonRampTexture(TextureID textureID);
    void SetDissolveTexture(TextureID textureID);
    void SetNormalMapTexture(TextureID textureID);

    // 色変換関係
    void SetColor(const Vector4& color);
    void SetOutlineColor(uint32_t color);

    // 単純なセッター
    void SetTransform(const WorldTransform& transform) { transform_ = transform; }
    void SetUvTransform(const WorldTransform& uvTransform) { uvTransform_ = uvTransform; }

    // 色の直接代入
    void SetColor(uint32_t color) { color_ = color; }
    void SetOutlineColor(const Vector4& color) { outlineColor_ = color; }

    // フラグ・パラメータ系
    void SetEasing(EasingType type) { easingType_ = type; }
    void SetEnableOutline(bool enable) { enableOutline_ = enable; }
    void SetOutlineWidth(float width) { outlineWidth_ = width; }
    void SetRenderGroup(RenderGroup group) { renderGroup_ = group; }

    // マテリアルパラメータ
    void SetEmissiveIntensity(float intensity) { materialHandle_.materialData->emissiveIntensity = intensity; }
    void SetEnableDissolve(bool enable) { materialHandle_.materialData->enableDissolve = enable; }

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }
    AnimationID GetCurrentAnimationID() const { return currentAnimationID_; }
    bool IsFinished() const { return isFinished_; }
    float GetAnimationTime() const { return animationTime_; }

    bool IsOutlineEnabled() const { return enableOutline_; }
    uint32_t GetColor() const { return color_; }

    // マテリアルデータへのアクセサ
    MaterialData* GetMaterial() { return materialHandle_.materialData; }
    const MaterialData* GetMaterial() const { return materialHandle_.materialData; }

    // ImGui用
    void UpdateUV()
    {
        uvTransform_.UpdateMatrix();
        // マテリアルデータへの転送
        if (materialHandle_.materialData)
        {
            materialHandle_.materialData->uvTransform = uvTransform_.matWorld_;
        }
    }
    // Transform
    WorldTransform* GetTransformPtr() { return &transform_; }
    WorldTransform* GetUVTransformPtr() { return &uvTransform_; }

    // マテリアル
    MaterialData* GetMaterialData() { return materialHandle_.materialData; }

    // 色・アウトライン
    uint32_t* GetColorPtr() { return &color_; }
    bool* GetEnableOutlinePtr() { return &enableOutline_; }
    float* GetOutlineWidthPtr() { return &outlineWidth_; }
    Vector4* GetOutlineColorPtr() { return &outlineColor_; }

    // テクスチャハンドル 
    uint32_t* GetTextureHandlePtr() { return &textureHandle_; }
    uint32_t* GetEnvMapTextureHandlePtr() { return &envMapTextureHandle_; }
    uint32_t* GetToonRampHandlePtr() { return &toonRampHandle_; }
    uint32_t* GetDissolveTextureHandlePtr() { return &dissolveTextureHandle_; }
    uint32_t* GetNormalMapHandlePtr() { return &normalMapHandle_; }

    // アニメーション制御
    float* GetSpeedScalePtr() { return &speedScale_; }
    bool* GetIsLoopPtr() { return &isLoop_; }

private:
    Engine* engine_ = nullptr;

    MaterialHandle materialHandle_;

    const Animation* currentAnimation_ = nullptr;
    bool isLoop_ = true;       // ループするか
    float speedScale_ = 1.0f;  // 再生速度
    bool isPlaying_ = false;   // 再生中かどうか

    AnimatedModelData animeModelData_;
    WorldTransform transform_;
    WorldTransform uvTransform_;

    Skeleton skeleton_;
    SkinCluster skinCluster_;
    float animationTime_;

    // テクスチャハンドル
    uint32_t textureHandle_ = 0;
    uint32_t envMapTextureHandle_ = 0;
    uint32_t toonRampHandle_ = 0;
    uint32_t dissolveTextureHandle_ = 0;
    uint32_t normalMapHandle_ = 0;

    // 色情報
    uint32_t color_ = 0xFFFFFFFF;

    // アニメーション制御
    bool isFinished_ = false;       // 再生が終了したか
    EasingType easingType_ = EasingType::EaseLinear;
    AnimationID currentAnimationID_ = AnimationID::count;

    // アウトライン
    bool enableOutline_ = false;
    float outlineWidth_ = 5.0f;
    Vector4 outlineColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };

    RenderGroup renderGroup_ = RenderGroup::Opaque;
};