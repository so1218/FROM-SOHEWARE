#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "ParticleEmitter.h"
#include "PropertyBinder.h"

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
    std::unique_ptr<Sprite> titleSprite_;

    std::unique_ptr<ParticleEmitter> titleSceneEmitter_ = nullptr;

    std::unique_ptr<PropertyBinder> binder_;
};
