#include "pch.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "AudioPlayer.h"
#include "Grid.h"

using namespace FE;

TitleScene::TitleScene(Engine* engine)
	: BaseScene(engine, "TitleScene")
{
	auto grid = std::make_unique<Grid>(engine_);

	objectManager_.AddObject(std::move(grid));

	titleSprite_ = std::make_unique<Sprite>(engine_);
}

void TitleScene::OnInitialize()
{
    camera_->Initialize();
    camera_->SetTranslation(Vector3(0, 0, -6.6f));


}

void TitleScene::OnUpdate()
{
	// シーン切り替えの入力検出
	if (Input::GetInstance().IsKeyTriggered(DIK_SPACE) || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonA)
        || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonB))
	{
		// シーンマネージャーを通じてシーン切り替えをリクエスト
		sceneManager_->RequestSceneChange(SceneID::Play);
	}
}

void TitleScene::OnDraw()
{
	titleSprite_->Draw();
}

void TitleScene::OnDebugDraw()
{
#ifdef ENABLE_IMGUI
#endif
}

void TitleScene::OnFinalize()
{
}