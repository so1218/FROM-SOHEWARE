#pragma once
#include "BaseScene.h"
#include "ParticleEmitter.h"
#include "PropertyBinder.h"

class TestScene : public FE::BaseScene
{
public:
    TestScene(FE::Engine* engine);

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
    std::unique_ptr<FE::PropertyBinder> binder_;
};
