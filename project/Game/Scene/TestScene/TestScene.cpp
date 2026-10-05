#include "pch.h"
#include "TestScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "Grid.h"

using namespace FE;

TestScene::TestScene(Engine* engine)
	: BaseScene(engine)
{
	auto grid = std::make_unique<Grid>(engine_);

	objectManager_.AddObject(std::move(grid));
}

void TestScene::OnInitialize()
{
	camera_->Initialize();
	camera_->SetTranslation(Vector3(0, 0, -6.6f));
}

void TestScene::OnUpdate()
{
	// シーン切り替えの入力検出
	if (Input::GetInstance().IsKeyTriggered(DIK_SPACE) || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonA)
		|| Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonB))
	{
		// シーンマネージャーを通じてシーン切り替えをリクエスト
		sceneManager_->RequestSceneChange(SceneID::Play);
	}
}

void TestScene::OnDraw()
{
}

void TestScene::OnDebugDraw()
{
#ifdef ENABLE_IMGUI

#endif
}

void TestScene::OnFinalize()
{}