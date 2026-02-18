#pragma once
#include "BaseScene.h"
#include "Player.h"
#include "ParticleEmitter.h"
#include "FollowCamera.h"
#include "Sprite.h"

class PlayScene : public BaseScene
{
public:
    PlayScene(Engine* engine);

    ~PlayScene();

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
};

