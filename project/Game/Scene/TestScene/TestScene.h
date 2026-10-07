#pragma once
#include "BaseScene.h"
#include "ParticleEmitter.h"

class TestScene : public FE::BaseScene
{
public:
    TestScene(FE::Engine* engine);

protected:
    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
};
