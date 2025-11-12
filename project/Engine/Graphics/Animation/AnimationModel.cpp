#include "AnimationModel.h"
#include "TimeManager.h"
#include "TextureHandle.h"

AnimationModel::AnimationModel(Engine* engine, Camera* camera, ModelData modelData, Animation animation)
    : engine_(engine), camera_(camera)
{
    animeModelData_.modelData = std::move(modelData);
    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());
    animeModelData_.animation = std::move(animation);
    skeleton_ = CreateSkeleton(animeModelData_.modelData.rootNode);
    skinCluster_ = CreateSkinCluster(engine_->graphicsDevice_->GetDevice(),
        skeleton_, animeModelData_.modelData, engine_->srvManager_.get());

    animationTime_ = 0.0f;
    textureHandle_ = TextureHandle::Get(TextureID::white1x1);
    envMapTextureHandle_ = TextureHandle::Get(TextureID::skyboxCubemap);
    color_ = 0xFFFFFFFF;
}

AnimationModel::~AnimationModel()
{
    if (engine_->srvManager_ != nullptr)
    {
        engine_->srvManager_->FreeSRV(skinCluster_.paletteSrvIndex);
    }
}

void AnimationModel::Update(float targetDuration, bool isLoop)
{
    // 再生が既に終了している場合は何もしない
    if (isFinished_)
    {
        return;
    }

    // 再生速度を計算
    float speed = 1.0f;
    if (targetDuration > 0.0f)
    {
        speed = animeModelData_.animation.duration / targetDuration;
    }

    // デルタタイムに速度を乗算してアニメーション時間を進める
    animationTime_ += TimeManager::GetInstance()->GetDeltaTime() * speed;
    float linearT = 0.0f;

    if (animeModelData_.animation.duration > 0.0f)
    {
        linearT = animationTime_ / animeModelData_.animation.duration;
    }
    // ループするかどうかで時間を調整
    if (isLoop)
    {
        animationTime_ = fmod(animationTime_, animeModelData_.animation.duration);
        linearT = fmod(linearT, 1.0f);
    }
    else
    {
        if (linearT >= 1.0f)
        {
            // アニメーションの終端で時間を固定し、終了フラグを立てる
            linearT = 1.0f;
            animationTime_ = animeModelData_.animation.duration;
            isFinished_ = true;
        }
    }
    float easedT = Easing::Evaluate(easingType_, linearT);
    float easedAnimationTime = easedT * animeModelData_.animation.duration;

    // アニメーションを適用
    ApplyAnimation(skeleton_, animeModelData_.animation, easedAnimationTime);
    UpdateSkeleton(skeleton_);
    UpdateSkinCluster(skinCluster_, skeleton_);
}

void AnimationModel::Draw()
{
    transform_.UpdateMatrix();
    engine_->renderer_->DrawAnimationModel(transform_, *camera_, animeModelData_, skinCluster_, textureHandle_, envMapTextureHandle_, color_, materialHandle_);
}

void AnimationModel::ResetAnimation()
{
    animationTime_ = 0.0f;
    isFinished_ = false;
}