#include "AnimationModel.h"

AnimationModel::AnimationModel(Engine* engine, Camera* camera, ModelData modelData, Animation animation)
    : engine_(engine), camera_(camera)
{
    animeModelData_.modelData = std::move(modelData);
    animeModelData_.animation = std::move(animation);
    skeleton_ = CreateSkeleton(animeModelData_.modelData.rootNode);
    skinCluster_ = CreateSkinCluster(engine_->graphicDevice_->GetDevice(),
        skeleton_, animeModelData_.modelData, engine_->srvDescriptorHeap_, engine_->descriptorSizeSRV_, engine_->srvAllocator_.get());

    animationTime_ = 0.0f;
    textureHandle_ = 0;
    color_ = 0xFFFFFFFF;
}

void AnimationModel::Update(float deltaTime)
{
    // アニメーション時間を進める（ループ再生）
    animationTime_ += deltaTime;
    animationTime_ = fmod(animationTime_, animeModelData_.animation.duration);

    // 内部で関連する更新関数を呼び出す
    /*ApplyAnimation(skeleton_, animeModelData_.animation, animationTime_);*/
    UpdateSkeleton(skeleton_);
    UpdateSkinCluster(skinCluster_, skeleton_);
}

void AnimationModel::Draw() 
{
    // ワールド変換行列の更新
    transform_.UpdateMatrix();
    // 描画関数
    engine_->DrawAnimationModel(transform_, *camera_, animeModelData_, skinCluster_, textureHandle_, color_);
}
