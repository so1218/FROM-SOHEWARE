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

void AnimationModel::Update(float speedScale, bool isLoop)
{
    // 再生が既に終了している場合は何もしない
    if (isFinished_)
    {
        return;
    }

    // 再生速度を計算
    float speed = 1.0f;
    if (speedScale > 0.0f)
    {
        speed = animeModelData_.currentAnimation->duration / speedScale;
    }

    // デルタタイムに速度を乗算してアニメーション時間を進める
    animationTime_ += TimeManager::GetInstance()->GetDeltaTime() * speed;
    float linearT = 0.0f;

    if (animeModelData_.currentAnimation->duration > 0.0f)
    {
        linearT = animationTime_ / animeModelData_.currentAnimation->duration;
    }
    // ループするかどうかで時間を調整
    if (isLoop)
    {
        animationTime_ = fmod(animationTime_, animeModelData_.currentAnimation->duration);
        linearT = fmod(linearT, 1.0f);
    }
    else
    {
        if (linearT >= 1.0f)
        {
            // アニメーションの終端で時間を固定し、終了フラグを立てる
            linearT = 1.0f;
            animationTime_ = animeModelData_.currentAnimation->duration;
            isFinished_ = true;
        }
    }
    float easedT = Easing::Evaluate(easingType_, linearT);
    float easedAnimationTime = easedT * animeModelData_.currentAnimation->duration;

    // アニメーションを適用
    if (animeModelData_.currentAnimation)
    {
        ApplyAnimation(skeleton_, *animeModelData_.currentAnimation, easedAnimationTime);
    }
    UpdateSkeleton(skeleton_);
    UpdateSkinCluster(skinCluster_, skeleton_);
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