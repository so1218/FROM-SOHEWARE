#include "pch.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "AudioPlayer.h"
#include "AmmoManager.h"
#include "EnemyManager.h"
#include "EnvironmentPropManager.h"
#include "WeatherEffectManager.h"
#include "PebbleField.h"
#include "FoliageField.h"
#include "GameUI.h"
#include "WaterManager.h"

using namespace FE;

TitleScene::TitleScene(Engine* engine)
	: BaseScene(engine, "TitleScene")
{
    player_ = objectManager_.Create<Player>(engine_, camera_.get());
    ground_ = objectManager_.Create<Ground>(engine_);
    grassField_ = objectManager_.Create<GrassField>(engine_);
    grassField_->SetTerrain(ground_->GetTerrain());
    treeField_ = objectManager_.Create<TreeField>(engine_);
    treeField_->SetTerrain(ground_->GetTerrain());
    objectManager_.Create<WeatherEffectManager>(engine_, camera_.get(), player_, ground_->GetTerrain());
    objectManager_.Create<EnemyManager>(engine_, "GameEnemy");
    objectManager_.Create<EnvironmentPropManager>(engine_, "EnvironmentProps");
    objectManager_.Create<PebbleField>(engine_);
    objectManager_.Create<FoliageField>(engine_);
    objectManager_.Create<WaterManager>(engine_, "GameWater");

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