#pragma once
#include "BaseScene.h"
#include "Player.h"
#include "FollowCamera.h"
#include "CameraRail.h"
#include "ParticleEmitter.h"
#include "Ground.h"
#include "GrassField.h"
#include "TreeField.h"

class PlayScene : public FE::BaseScene
{
public:
    PlayScene(FE::Engine* engine);

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
    Player* player_ = nullptr;
    Ground* ground_ = nullptr;
    GrassField* grassField_ = nullptr;
    TreeField* treeField_ = nullptr;
    std::unique_ptr<FollowCamera> followCamera_;
    std::unique_ptr<FE::CameraRail> openingRail_;

};

