#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Sprite.h"
#include "PropertyBinder.h"

class GameUI : public FS::GameObject
{
public:
    GameUI(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

private:
    FS::Engine* engine_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    // 8枚のスプライトを管理
    std::unique_ptr<FS::Sprite> spriteCameraSwitch_;
    std::unique_ptr<FS::Sprite> spriteMainCameraMode_;
    std::unique_ptr<FS::Sprite> spriteMove_;
    std::unique_ptr<FS::Sprite> spriteMainCameraMove_;
    std::unique_ptr<FS::Sprite> spriteDebugCameraMode_;
    std::unique_ptr<FS::Sprite> spriteDebugCameraTranslate_;
    std::unique_ptr<FS::Sprite> spriteDebugCameraRotate_;
    std::unique_ptr<FS::Sprite> spriteDebugCameraZoom_;

    // まとめて表示/非表示を切り替えるフラグ
    bool isVisible_ = true;
};