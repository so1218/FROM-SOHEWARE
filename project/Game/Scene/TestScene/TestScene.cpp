#include "pch.h"
#include "TestScene.h"
#include "SceneManager.h"
#include "GameDefine.h"
#include "GameInput.h"
#include "Grid.h"
#include "AudioPlayer.h"

using namespace FS;

TestScene::TestScene(Engine* engine)
	: BaseScene(engine, SceneName::Test)
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
	if (GameInput::IsTriggered(GameAction::Confirm) || GameInput::IsTriggered(GameAction::Cancel))
	{
		// シーンマネージャーを通じてシーン切り替えをリクエスト
		sceneManager_->RequestSceneChange(SceneName::Play);
	}
}

void TestScene::OnDraw()
{
}

void TestScene::OnDebugGui()
{

}

void TestScene::OnFinalize()
{
}