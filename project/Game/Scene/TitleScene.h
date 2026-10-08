#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include "Ground.h"
#include "GrassField.h"
#include "TreeField.h"

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
    Player* player_ = nullptr;
    Ground* ground_ = nullptr;
    GrassField* grassField_ = nullptr;
    TreeField* treeField_ = nullptr;
    // メンバー変数
    std::unique_ptr<FE::Sprite> titleSprite_;
};
