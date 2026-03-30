#pragma once
#include "Vector3.h"
#include "WorldTransform.h"
#include "ParticleSystem.h"
#include "Model.h"
#include "AnimationModel.h"

namespace FE
{

class ParticleEmitter
{
public:
    void Initialize(const EmitterConfig& config);

    void Update(ParticleSystem& particleSystem);

    // Emitterの位置設定
    void SetTargetToFollow(WorldTransform* target);

    void SetFollowOffset(const Vector3& offset) { followOffset_ = offset; }
    void SetOffsetScale(const Vector3& scale) { offsetScale_ = scale; }
    void SetAttractionTarget(const WorldTransform* target) { attractionTarget_ = target; }
    void SetVortexTarget(const WorldTransform* target) { vortexTarget_ = target; }
    void SetPosition(const Vector3& position) { position_ = position; }

    void Play(); // エミッターの再生を開始/リスタート
    void Stop(); // エミッターの再生を停止

    void Destroy();

    void SetFollowAxes(bool x, bool y, bool z);

    void SetTargetModel(const Model* model)
    {
        targetModel_ = model;
        if (model)
        {
            SetTargetToFollow(const_cast<WorldTransform*>(&model->GetTransform()));
        }
    }

    void SetTargetAnimationModel(const AnimationModel* animModel)
    {
        targetAnimModel_ = animModel;
        if (animModel)
        {
            SetTargetToFollow(const_cast<WorldTransform*>(&animModel->GetTransform()));
        }
    }

    Vector3 position_;
    float spawnInterval_;
    float lifetime_;
    float timeSinceLastSpawn_;
    int amount_;
    float duration_;
    bool looping_;
    bool isPlaying_ = false;  // 現在再生中か
    float elapsedTime_ = 0.0f;// 再生開始からの経過時間
    WorldTransform* targetToFollow_ = {};
    Vector3 followOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 offsetScale_ = { 1.0f, 1.0f, 1.0f };
    const WorldTransform* attractionTarget_ = nullptr;
    const WorldTransform* vortexTarget_ = nullptr;

    std::string name_ = "Emitter";
    std::string& presetName_ = name_;

    EmitterConfig emitterConfig_;
    ParticleConfig particleConfig_;

    const Model* targetModel_ = nullptr;
    const AnimationModel* targetAnimModel_ = nullptr;

    bool isDead_ = false;

    bool followX_ = true;
    bool followY_ = true;
    bool followZ_ = true;
};

}


