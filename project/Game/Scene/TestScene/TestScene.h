#pragma once
#include "BaseScene.h"
#include "ParticleEmitter.h"

class TestScene : public FS::BaseScene
{
public:
    TestScene(FS::Engine* engine);

protected:
    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugGui() override;
    void OnFinalize() override;

private:
};
