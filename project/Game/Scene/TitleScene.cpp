#include "TitleScene.h"
#include "SceneManager.h"
#include "PlayScene.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "AudioPlayer.h"
#include "TimeManager.h"
#include "AudioHandle.h"

using namespace FromEngine;

TitleScene::TitleScene(Engine* engine, Camera* camera)
    : BaseScene(engine, camera)
{
    sprite_ = std::make_unique<Sprite>(engine_);
    spriteUse_ = std::make_unique<Sprite>(engine_);
    spritePress_ = std::make_unique<Sprite>(engine_);
}

void TitleScene::OnInitialize()
{
    camera_->Initialize();
    camera_->SetTranslation(Vector3(0, 0, -6.6f));

    spritePos_ = { 640, 227 };
    sprite_->SetPosition(spritePos_);
    spriteSize_ = { 1000.0f, 300.0f }; 
    sprite_->SetSize(spriteSize_);
    sprite_->SetAnchorPoint({ 0.5f, 0.5f });
    sprite_->SetTexture(TextureID::title);

    spritePosUse_ = { 640, 457 };
    spriteUse_->SetPosition(spritePosUse_);
    spriteSizeUse_ = { 473.0f, 105.0f };
    spriteUse_->SetSize(spriteSizeUse_);
    spriteUse_->SetAnchorPoint({ 0.5f, 0.5f });
    spriteUse_->SetTexture(TextureID::useController);

    spritePosPress_ = { 640, 564 };
    spritePress_->SetPosition(spritePosPress_);
    spriteSizePress_ = { 757.0f, 153.0f };
    spritePress_->SetSize(spriteSizePress_);
    spritePress_->SetAnchorPoint({ 0.5f, 0.5f });
    spritePress_->SetTexture(TextureID::pressSousa);

    titleEmitter_ = engine_->particleSystem_->CreateEmitter("title");
    engine_->particleSystem_->AddEmitter(std::move(titleEmitter_));

    engine_->postEffectManager_->GetPostEffectData()->modeFlags[0] |= VIGNETTE;
    engine_->postEffectManager_->GetPostEffectData()->vignetteAmount = 1.29f;
    engine_->postEffectManager_->GetPostEffectData()->vignetteRadius = 0.029f;
    engine_->postEffectManager_->GetPostEffectData()->vignetteSoftness = 0.723f;
    engine_->postEffectManager_->GetPostEffectData()->vignetteEllipseScale = { 1.05f,0.95f };
    engine_->postEffectManager_->GetPostEffectData()->vignetteColor = { 6.0f / 255.0f,42.0f / 255.0f,72.0f / 255.0f };
    engine_->postEffectManager_->GetPostEffectData()->modeFlags[0] |= COLOR_TINT;
    engine_->postEffectManager_->GetPostEffectData()->tintColor = { 130.0f / 255.0f,255.0f / 255.0f,241.0f / 255.0f };
    engine_->postEffectManager_->GetPostEffectData()->tintMulColorAmount = 0.015f;
    engine_->postEffectManager_->GetPostEffectData()->tintAddColorAmount = 0.075f;
    engine_->postEffectManager_->GetPostEffectData()->tintScreenColorAmount = 0.25f;
    engine_->postEffectManager_->GetCombineSettings()->enableFog = true;
    engine_->postEffectManager_->GetCombineSettings()->fogEnd = 5000.0f;
    engine_->postEffectManager_->GetCombineSettings()->fogStart = 20.0f;
    engine_->postEffectManager_->GetCombineSettings()->fogColor = { 86.0f / 255.0f,175.0f / 255.0f,254.0f / 255.0f };

    AudioPlayer::GetInstance().StopUnique(AudioHandle::Get(AudioID::clearSceneBGM));
    AudioPlayer::GetInstance().StopUnique(AudioHandle::Get(AudioID::playSceneBGM));

}

void TitleScene::OnUpdate()
{
    AudioPlayer::GetInstance().PlayUnique(AudioHandle::Get(AudioID::titleSceneBGM), true, 20);

	// シーン切り替えの入力検出
	if (Input::GetInstance().IsKeyTriggered(DIK_SPACE) || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonA)
        || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonB))
	{
		// シーンマネージャーを通じてシーン切り替えをリクエスト
		sceneManager_->RequestSceneChange(SceneID::Sample);
        AudioPlayer::GetInstance().PlayUnique(AudioHandle::Get(AudioID::dicision), false, 100);
	}
}

void TitleScene::OnDraw()
{
    sprite_->Draw();
    spriteUse_->Draw();
    spritePress_->Draw();
}

void TitleScene::OnDebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("タイトルシーン");

    if (ImGui::DragFloat2("Sprite Pos", &spritePos_.x, 1.0f))
    {
        sprite_->SetPosition(spritePos_);
    }
    if (ImGui::DragFloat2("Sprite Size", &spriteSize_.x, 1.0f))
    {
        sprite_->SetSize(spriteSize_);
    }
    if (ImGui::DragFloat2("Sprite Pos Use", &spritePosUse_.x, 1.0f))
    {
        spriteUse_->SetPosition(spritePosUse_);
    }
    if (ImGui::DragFloat2("Sprite Size Use", &spriteSizeUse_.x, 1.0f))
    {
        spriteUse_->SetSize(spriteSizeUse_);
    }

    if (ImGui::DragFloat2("Sprite Pos Press", &spritePosPress_.x, 1.0f))
    {
        spritePress_->SetPosition(spritePosPress_);
    }
    if (ImGui::DragFloat2("Sprite Size Press", &spriteSizePress_.x, 1.0f))
    {
        spritePress_->SetSize(spriteSizePress_);
    }


    ImGui::End();
#endif
}

void TitleScene::OnFinalize()
{
}