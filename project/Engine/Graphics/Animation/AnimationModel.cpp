#include "AnimationModel.h"
#include "TimeManager.h"


AnimationModel::AnimationModel(Engine* engine, Camera* camera, ModelData modelData, Animation animation)
    : engine_(engine), camera_(camera)
{
    animeModelData_.modelData = std::move(modelData);
    materialHandle_ = engine_->materialManager_->CreateMaterial(engine_->graphicDevice_->GetDevice());
    animeModelData_.animation = std::move(animation);
    skeleton_ = CreateSkeleton(animeModelData_.modelData.rootNode);
    skinCluster_ = CreateSkinCluster(engine_->graphicDevice_->GetDevice(),
        skeleton_, animeModelData_.modelData, engine_->srvDescriptorHeap_, engine_->descriptorSizeSRV_, engine_->srvAllocator_.get());

    animationTime_ = 0.0f;
    textureHandle_ = 0;
    color_ = 0xFFFFFFFF;
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

    // ループするかどうかで時間を調整
    if (isLoop)
    {
        animationTime_ = fmod(animationTime_, animeModelData_.animation.duration);
    }
    else
    {
        if (animationTime_ >= animeModelData_.animation.duration)
        {
            // アニメーションの終端で時間を固定し、終了フラグを立てる
            animationTime_ = animeModelData_.animation.duration;
            isFinished_ = true;
        }
    }

    // アニメーションを適用
    ApplyAnimation(skeleton_, animeModelData_.animation, animationTime_);
    UpdateSkeleton(skeleton_);
    UpdateSkinCluster(skinCluster_, skeleton_);
}

void AnimationModel::Draw()
{
    // ワールド変換行列の更新
    transform_.UpdateMatrix();
    // 描画関数
    engine_->DrawAnimationModel(transform_, *camera_, animeModelData_, skinCluster_, textureHandle_, color_, materialHandle_);
}

void AnimationModel::ResetAnimation()
{
    animationTime_ = 0.0f;
    isFinished_ = false;
}