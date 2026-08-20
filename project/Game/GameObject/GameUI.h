#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Sprite.h"
#include "PropertyBinder.h"

class GameUI : public FE::GameObject
{
public:
    GameUI(FE::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

private:
    FE::Engine* engine_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    // 8枚のスプライトを管理
    std::unique_ptr<FE::Sprite> spriteCameraSwitch_;
    std::unique_ptr<FE::Sprite> spriteMainCameraMode_;
    std::unique_ptr<FE::Sprite> spriteMove_;
    std::unique_ptr<FE::Sprite> spriteMainCameraMove_;
    std::unique_ptr<FE::Sprite> spriteDebugCameraMode_;
    std::unique_ptr<FE::Sprite> spriteDebugCameraTranslate_;
    std::unique_ptr<FE::Sprite> spriteDebugCameraRotate_;
    std::unique_ptr<FE::Sprite> spriteDebugCameraZoom_;

    // まとめて表示/非表示を切り替えるフラグ
    bool isVisible_ = true;
};