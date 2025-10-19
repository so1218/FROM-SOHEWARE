#pragma once

#include "AnimationData.h" 
#include "Engine.h"

class AnimationModel
{
public:
    // コンストラクタ
    AnimationModel(Engine* engine, Camera* camera, ModelData modelData, Animation animation);

    // 更新処理
    void Update(float targetDuration, bool isLoop);

    // 描画処理
    void Draw();

    // アニメーションの状態をリセット
    void ResetAnimation();

    // トランスフォームへの参照を取得
    WorldTransform& GetTransform() { return transform_; }


    Engine* engine_ = nullptr;
    Camera* camera_ = nullptr;

    AnimatedModelData animeModelData_;
    WorldTransform transform_;

    Skeleton skeleton_;
    SkinCluster skinCluster_;
    float animationTime_;

    uint32_t textureHandle_;
    uint32_t color_;

    bool isFinished_ = false;      // 再生が終了したか
};