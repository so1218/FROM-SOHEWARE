#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "ParticleEmitter.h"

class TitleScene : public FE::BaseScene
{
public:
    TitleScene(FE::Engine* engine);

protected:
    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
    std::unique_ptr<FE::Sprite> titleSprite_;
};
