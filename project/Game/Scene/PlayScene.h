#pragma once
#include "BaseScene.h"
#include "FollowCamera.h"
#include "ParticleEmitter.h"

class PlayScene : public FS::BaseScene
{
public:
    PlayScene(FS::Engine* engine);

protected:
    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
    std::unique_ptr<FollowCamera> followCamera_;
};

