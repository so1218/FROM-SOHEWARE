#include "AnimationModel.h"
#include "TimeManager.h"
#include "Engine.h"

AnimationModel::AnimationModel(Engine* engine, const ModelData* modelData, const Animation* animation)
    : engine_(engine)
{
    assert(modelData != nullptr);
    assert(animation != nullptr);

    animeModelData_.modelData = modelData;
    animeModelData_.currentAnimation = animation;

    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());
    skeleton_ = CreateSkeleton(animeModelData_.modelData->rootNode);
    skinCluster_ = CreateSkinCluster(engine_->graphicsDevice_->GetDevice(),
        skeleton_, *animeModelData_.modelData, engine_->srvManager_.get());

    animationTime_ = 0.0f;

    // 初期テクスチャ設定
    textureHandle_ = TextureHandle::Get(TextureID::white1x1);
    envMapTextureHandle_ = TextureHandle::Get(TextureID::skyboxCubemap);
    toonRampHandle_ = TextureHandle::Get(TextureID::toonRamp);
    dissolveTextureHandle_ = TextureHandle::Get(TextureID::white1x1);
    normalMapHandle_ = TextureHandle::Get(TextureID::white1x1);

    color_ = 0xFFFFFFFF;
}

AnimationModel::~AnimationModel()
{
    if (engine_->srvManager_ != nullptr)
    {
        engine_->srvManager_->FreeSRV(skinCluster_.paletteSrvIndex);
    }
}

void AnimationModel::SetTexture(TextureID textureID) { textureHandle_ = TextureHandle::Get(textureID); }
void AnimationModel::SetEnvironmentMapTexture(TextureID textureID) { envMapTextureHandle_ = TextureHandle::Get(textureID); }
void AnimationModel::SetToonRampTexture(TextureID textureID) { toonRampHandle_ = TextureHandle::Get(textureID); }
void AnimationModel::SetDissolveTexture(TextureID textureID) { dissolveTextureHandle_ = TextureHandle::Get(textureID); }
void AnimationModel::SetNormalMapTexture(TextureID textureID) { normalMapHandle_ = TextureHandle::Get(textureID); }
void AnimationModel::SetColor(const Vector4& color) { color_ = Math::ColorVectorToUint32(color); }
void AnimationModel::SetOutlineColor(uint32_t color) { outlineColor_ = Math::Uint32ToColorVector(color); }

// アニメーション制御
void AnimationModel::ResetAnimation()
{
    animationTime_ = 0.0f;
    isFinished_ = false;
}

void AnimationModel::SetAnimation(const Animation* animation)
{
    // アニメーションデータを上書きコピー
    animeModelData_.currentAnimation = animation;
    // 再生時間をリセット
    ResetAnimation();
}

// 毎フレームの更新
void AnimationModel::Update()
{
    // 再生中でない、または終了している場合は、姿勢更新のみ行い終了
    if (!isPlaying_ || isFinished_)
    {
        UpdateSkeleton(skeleton_);
        UpdateSkinCluster(skinCluster_, skeleton_);
        return;
    }

    // アニメーションデータがない、または長さが0なら処理しない
    if (!animeModelData_.currentAnimation || animeModelData_.currentAnimation->duration <= 0.0f) {
        return;
    }

    float duration = animeModelData_.currentAnimation->duration;

    // 速度の計算
    animationTime_ += TimeManager::GetInstance()->GetDeltaTime() * speedScale_;

    // 進行度（0.0～1.0）の計算
    float linearT = animationTime_ / duration;

    // ループと終了判定
    if (isLoop_)
    {
        // 1.0を超えたら0.0に戻る
        linearT = fmod(linearT, 1.0f);
        animationTime_ = fmod(animationTime_, duration);
    }
    else
    {
        // 1.0でカンストし、終了フラグを立てる
        if (linearT >= 1.0f)
        {
            linearT = 1.0f;
            animationTime_ = duration;
            isFinished_ = true;
            isPlaying_ = false;
        }
    }

    // イージングの適用
    float easedT = Easing::Evaluate(easingType_, linearT);

    // イージングされたTを、実際のアニメーション時間に戻す
    float playbackTime = easedT * duration;

    // アニメーション適用
    ApplyAnimation(skeleton_, *animeModelData_.currentAnimation, playbackTime);

    // 行列更新
    UpdateSkeleton(skeleton_);
    UpdateSkinCluster(skinCluster_, skeleton_);
}

// アニメーション再生の開始
void AnimationModel::Play(const Animation* animation, bool isLoop, float speedScale)
{
    // アニメーション切り替え
    animeModelData_.currentAnimation = animation;

    // 設定をメンバ変数に保存
    isLoop_ = isLoop;
    speedScale_ = speedScale;

    // 時間リセット
    ResetAnimation();
    isPlaying_ = true;
}

void AnimationModel::Draw()
{
    transform_.UpdateMatrix();

    engine_->renderer_->SubmitAnimationModel(
        transform_,
        animeModelData_,
        skinCluster_,
        textureHandle_,
        envMapTextureHandle_,
        toonRampHandle_,
        dissolveTextureHandle_,
        normalMapHandle_,
        color_,
        materialHandle_,
        enableOutline_,
        outlineWidth_,
        outlineColor_,
        renderGroup_
    );
}