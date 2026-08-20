#include "pch.h"
#include "GameUI.h"

using namespace FE;

GameUI::GameUI(Engine* engine) : engine_(engine)
{
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, "GameUI");

    spriteCameraSwitch_ = std::make_unique<Sprite>(engine_);
    spriteMainCameraMode_ = std::make_unique<Sprite>(engine_);
    spriteMove_ = std::make_unique<Sprite>(engine_);
    spriteMainCameraMove_ = std::make_unique<Sprite>(engine_);
    spriteDebugCameraMode_ = std::make_unique<Sprite>(engine_);
    spriteDebugCameraTranslate_ = std::make_unique<Sprite>(engine_);
    spriteDebugCameraRotate_ = std::make_unique<Sprite>(engine_);
    spriteDebugCameraZoom_ = std::make_unique<Sprite>(engine_);
}

void GameUI::Initialize()
{
    binder_->Bind("IsVisible", &isVisible_, true);

    binder_->BindSprite("SpriteCameraSwitch", spriteCameraSwitch_.get());
    binder_->BindSprite("SpriteMainCameraMode", spriteMainCameraMode_.get());
    binder_->BindSprite("SpriteMove", spriteMove_.get());
    binder_->BindSprite("SpriteMainCameraMove", spriteMainCameraMove_.get());
    binder_->BindSprite("SpriteDebugCameraMode", spriteDebugCameraMode_.get());
    binder_->BindSprite("SpriteDebugCameraTranslate", spriteDebugCameraTranslate_.get());
    binder_->BindSprite("SpriteDebugCameraRotate", spriteDebugCameraRotate_.get());
    binder_->BindSprite("SpriteDebugCameraZoom", spriteDebugCameraZoom_.get());
}

void GameUI::Update()
{
}

void GameUI::Draw()
{
    if (!isVisible_) return;

    spriteCameraSwitch_->Draw();
    spriteMainCameraMode_->Draw();
    spriteMove_->Draw();
    spriteMainCameraMove_->Draw();
    spriteDebugCameraMode_->Draw();
    spriteDebugCameraTranslate_->Draw();
    spriteDebugCameraRotate_->Draw();
    spriteDebugCameraZoom_->Draw();
}

void GameUI::DebugDraw()
{
#if ENABLE_IMGUI
    ImGui::Begin("ゲームUI調整");
    binder_->Draw("IsVisible", "表示切り替え");
    ImGui::Separator();
    binder_->DrawSprite("SpriteCameraSwitch", "CameraSwitchインスペクター");
    binder_->DrawSprite("SpriteMainCameraMode", "MainCameraModeインスペクター");
    binder_->DrawSprite("SpriteMove", "Moveインスペクター");
    binder_->DrawSprite("SpriteMainCameraMove", "MainCameraMoveインスペクター");
    binder_->DrawSprite("SpriteDebugCameraMode", "DebugCameraModeインスペクター");
    binder_->DrawSprite("SpriteDebugCameraTranslate", "DebugCameraTranslateインスペクター");
    binder_->DrawSprite("SpriteDebugCameraRotate", "DebugCameraRotateインスペクター");
    binder_->DrawSprite("SpriteDebugCameraZoom", "DebugCameraZoomインスペクター");
    ImGui::End();
#endif
}