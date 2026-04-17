#pragma once
#include "BaseScene.h"
#include "Player.h"
#include "FollowCamera.h"
#include "CameraRail.h"
#include "ParticleEmitter.h"
#include "Bubble.h"

class TestSceneHori : public FE::BaseScene
{
public:
    TestSceneHori(FE::Engine* engine);

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
    Player* player_ = nullptr;
    Bubble* bubble_ = nullptr;
    std::unique_ptr<FollowCamera> followCamera_;
    std::unique_ptr<FE::CameraRail> openingRail_;

    std::unique_ptr<FE::ParticleEmitter> testSceneEmitter_ = nullptr;
    std::unique_ptr<FE::ParticleEmitter> auraEmitter_ = nullptr;
};

