#pragma once
#include "BaseScene.h"

class MainScene : public FS::BaseScene
{
public:
    MainScene(FS::Engine* engine);

protected:
    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugGui() override;
    void OnFinalize() override;
};
