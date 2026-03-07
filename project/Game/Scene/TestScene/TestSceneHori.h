#pragma once
#include "BaseScene.h"
#include "Player.h"
#include "FollowCamera.h"
#include "ParticleEmitter.h"
#include "Bubble.h"

class TestSceneHori : public BaseScene
{
public:
    TestSceneHori(Engine* engine);

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
    Player* player_ = nullptr;
    Bubble* bubble_ = nullptr;
    FollowCamera* followCamera_ = nullptr;

    std::unique_ptr<ParticleEmitter> testSceneEmitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> auraEmitter_ = nullptr;
};

