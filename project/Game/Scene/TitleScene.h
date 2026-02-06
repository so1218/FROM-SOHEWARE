#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "ParticleEmitter.h"

class TitleScene : public BaseScene
{
public:
    TitleScene(Engine* engine);

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
};
