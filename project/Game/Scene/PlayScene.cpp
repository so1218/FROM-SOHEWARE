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
#include "TitleMenu.h"

using namespace FS;

PlayScene::PlayScene(Engine* engine)
    : BaseScene(engine, SceneName::Play)
{
	// ゲームオブジェクトの生成・登録
    auto player = objectManager_.Create<Player>(engine_, camera_.get());
    player_ = player;
    followCamera_ = std::make_unique<FollowCamera>(engine_, &player->GetTransform());
    titleCamera_ = std::make_unique<TitleCamera>(engine_, followCamera_.get());
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
    enemyManager_ = objectManager_.Create<EnemyManager>(engine_, "GameEnemy");
    objectManager_.Create<EnvironmentPropManager>(engine_, "EnvironmentProps");
    objectManager_.Create<FoliageField>(engine_);
    gameUI_ = objectManager_.Create<GameUI>(engine_);
    tutorial_ = objectManager_.Create<Tutorial>(engine_, player);
    titleMenu_ = objectManager_.Create<TitleMenu>(engine_);
    objectManager_.Create<WaterManager>(engine_, "GameWater");
}

void PlayScene::OnInitialize()
{
    followCamera_->Initialize();
    titleCamera_->Initialize();
    // プレイヤーや敵の初期化はこのあと各 Initialize で行われる
    EnterTitle();
}

void PlayScene::OnUpdate()
{
    switch (phase_)
    {
    case Phase::Title:
        if (titleMenu_->HasStarted())
        {
            titleCamera_->StartTransition(camera_.get());
            phase_ = Phase::CameraMove;
        }
        break;

    case Phase::CameraMove:
        if (titleCamera_->IsTransitionFinished())
        {
            StartPlaying();
        }
        break;

    case Phase::Playing:
        break;
    }
}

void PlayScene::StartPlaying()
{
    cameraManager_->ChangeController(followCamera_.get());
    player_->SetControllable(true);
    gameUI_->SetActive(true);
    tutorial_->Begin();
    phase_ = Phase::Playing;
}

void PlayScene::EnterTitle()
{
    titleCamera_->CancelTransition();
    cameraManager_->ChangeController(titleCamera_.get());
    player_->SetControllable(false);
    gameUI_->SetActive(false);
    titleMenu_->Restart();
    phase_ = Phase::Title;
}

void PlayScene::ReturnToTitle()
{
    player_->ResetToSpawn();
    enemyManager_->ResetEnemies();
    tutorial_->Stop();
    EnterTitle();
}

void PlayScene::OnDraw()
{
}

void PlayScene::OnDebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::Begin("タイトル");
    ImGui::SeparatorText("カメラ");
    titleCamera_->DrawProperties();

    ImGui::SeparatorText("確認用");
    if (phase_ != Phase::Playing && ImGui::Button("タイトルを飛ばす"))
    {
        titleMenu_->Skip();
        StartPlaying();
    }
    if (phase_ == Phase::Playing && ImGui::Button("タイトルに戻る"))
    {
        ReturnToTitle();
    }
    ImGui::End();
#endif
}

void PlayScene::OnFinalize()
{
}
