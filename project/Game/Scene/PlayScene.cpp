#include "pch.h"
#include "PlayScene.h"
#include "GameDefine.h"
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
#include "FoliageField.h"
#include "GameUI.h"
#include "WaterManager.h"
#include "Ground.h"
#include "TreeField.h"
#include "Player.h"
#include "Tutorial.h"

using namespace FS;

PlayScene::PlayScene(Engine* engine)
    : BaseScene(engine, SceneName::Play)
{
	// ゲームオブジェクトの生成・登録
    auto player = objectManager_.Create<Player>(engine_, camera_.get());
    followCamera_ = std::make_unique<FollowCamera>(engine_, &player->GetTransform());
    auto ground = objectManager_.Create<Ground>(engine_);
    auto grassField = objectManager_.Create<GrassField>(engine_);
    grassField->SetTerrain(ground->GetTerrain());
    auto treeField = objectManager_.Create<TreeField>(engine_);
    treeField->SetTerrain(ground->GetTerrain());
    player->SetTerrain(ground->GetTerrain());
    player->SetTreeField(treeField);
    followCamera_->SetTerrain(ground->GetTerrain());
    player->SetFollowCamera(followCamera_.get());
	auto wetherEffectManager = objectManager_.Create<WeatherEffectManager>(engine_, camera_.get(), player, ground->GetTerrain());
	wetherEffectManager->SetCameraManager(cameraManager_.get());
    objectManager_.Create<AmmoManager>(engine_, "GameAmmo");
    objectManager_.Create<EnemyManager>(engine_, "GameEnemy");
    objectManager_.Create<EnvironmentPropManager>(engine_, "EnvironmentProps");
    objectManager_.Create<FoliageField>(engine_);
    objectManager_.Create<GameUI>(engine_);
    objectManager_.Create<Tutorial>(engine_, player);
    /*objectManager_.Create<WaterManager>(engine_, "GameWater");*/
}

void PlayScene::OnInitialize()
{
    // デフォルトカメラの設定
    followCamera_->Initialize();
    cameraManager_->ChangeController(followCamera_.get());
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
