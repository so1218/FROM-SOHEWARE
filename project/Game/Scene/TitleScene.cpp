#include "TitleScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "AudioPlayer.h"
#include "TimeManager.h"
#include "Grid.h"

TitleScene::TitleScene(Engine* engine)
    : BaseScene(engine)
{
	auto grid = std::make_unique<Grid>(engine_);

	objectManager_.AddObject(std::move(grid));

	titleSprite_ = std::make_unique<Sprite>(engine_);

	binder_ = std::make_unique<PropertyBinder>(engine_, "Title");
}

void TitleScene::OnInitialize()
{
    camera_->Initialize();
    camera_->SetTranslation(Vector3(0, 0, -6.6f));

	binder_->BindSprite("TitleSprite", titleSprite_.get());

	titleSceneEmitter_ = engine_->particleSystem_->CreateEmitter("titleScene");
	engine_->particleSystem_->AddEmitter(std::move(titleSceneEmitter_));
}

void TitleScene::OnUpdate()
{
	// シーン切り替えの入力検出
	if (Input::GetInstance().IsKeyTriggered(DIK_SPACE) || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonA)
        || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonB))
	{
		// シーンマネージャーを通じてシーン切り替えをリクエスト
		sceneManager_->RequestSceneChange(SceneID::TestHori);
	}
}

void TitleScene::OnDraw()
{
	titleSprite_->Draw();
}

void TitleScene::OnDebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("タイトルシーン");
	binder_->DrawSprite("TitleSprite", "タイトルスプライトインスペクター");
    ImGui::End();
#endif
}

void TitleScene::OnFinalize()
{
}