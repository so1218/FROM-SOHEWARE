#include "TitleScene.h"
#include "SceneManager.h"
#include "PlayScene.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "AudioPlayer.h"
#include "TimeManager.h"
#include "AudioHandle.h"

using namespace FromEngine;

TitleScene::TitleScene(Engine* engine)
    : BaseScene(engine)
{
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
		sceneManager_->RequestSceneChange(SceneID::Sample);
	}
}

void TitleScene::OnDraw()
{

}

void TitleScene::OnDebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("タイトルシーン");

    ImGui::End();
#endif
}

void TitleScene::OnFinalize()
{
}