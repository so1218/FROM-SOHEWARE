#pragma once
#include "BaseScene.h"
#include "Player.h"
#include "Enemy.h"
#include "ShakeEffect.h"
#include "FollowCamera.h"
#include "ParticleEmitter.h"

class SampleSceneHori : public BaseScene
{
public:
	SampleSceneHori(Engine* engine);

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
    Player* player_ = nullptr;
    FollowCamera* followCamera_ = nullptr;
};

