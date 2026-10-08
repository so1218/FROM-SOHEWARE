#include "pch.h"
#include "PlayScene.h"
#include "ImGuiManager.h"
#include "TimeManager.h"
#include "Input.h"
#include "GrassField.h"
#include "SceneManager.h"
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

PlayScene::PlayScene(Engine* engine)
    : BaseScene(engine, "PlayScene")
{
	// ゲームオブジェクトの生成・登録
    player_ = objectManager_.Create<Player>(engine_, camera_.get());
    followCamera_ = std::make_unique<FollowCamera>(engine_, &player_->GetTransform());
    ground_ = objectManager_.Create<Ground>(engine_);
    grassField_ = objectManager_.Create<GrassField>(engine_);
    grassField_->SetTerrain(ground_->GetTerrain());
    treeField_ = objectManager_.Create<TreeField>(engine_);
    treeField_->SetTerrain(ground_->GetTerrain());
    player_->SetTerrain(ground_->GetTerrain());
    player_->SetTreeField(treeField_);
    followCamera_->SetTerrain(ground_->GetTerrain());
	player_->SetFollowCamera(followCamera_.get());
    objectManager_.Create<WeatherEffectManager>(engine_, camera_.get(), player_, ground_->GetTerrain());
    objectManager_.Create<AmmoManager>(engine_, "GameAmmo");
    objectManager_.Create<EnemyManager>(engine_, "GameEnemy");
    objectManager_.Create<EnvironmentPropManager>(engine_, "EnvironmentProps");
    objectManager_.Create<PebbleField>(engine_);
    objectManager_.Create<FoliageField>(engine_);
    objectManager_.Create<GameUI>(engine_);
    objectManager_.Create<WaterManager>(engine_, "GameWater");
}

void PlayScene::OnInitialize()
{
    // ライトの設定
    engine_->GetLightManager()->GetDirectionalLightData()[0].enable = true;
    engine_->GetLightManager()->GetDirectionalLightData()[0].direction = { 2.6f,-0.4f,1.4f };
    engine_->GetLightManager()->GetDirectionalLightData()[0].intensity = 0.4f;
    engine_->GetLightManager()->GetDirectionalLightData()[0].volumetricScatteringIntensity = 13.0f;

    // デフォルトカメラの設定
    followCamera_->Initialize();
    cameraManager_->ChangeController(followCamera_.get());

	AudioPlayer::GetInstance().PlayUnique("playSceneBGM", true, 50);
}

void PlayScene::OnUpdate()
{


}

void PlayScene::OnDraw()
{
   
}

void PlayScene::OnDebugDraw()
{
#ifdef ENABLE_IMGUI


#endif
}

void PlayScene::OnFinalize()
{
}
