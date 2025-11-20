#pragma once
#include "BaseScene.h"
#include "Engine.h"
#include "Sprite.h"
#include "GameObjectManager.h"

class TitleScene : public BaseScene
{
public:
    TitleScene(Engine* engine, Camera* camera);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;
    void Finalize() override;

    // メンバー変数
    Engine* engine_;
    Camera* camera_;

    GameObjectManager objectManager_;

    std::unique_ptr<Sprite> sprite_;
};
