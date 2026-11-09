#include "pch.h"
#include "PlayScene.h"
#include "GameDefine.h"
#include "ImGuiManager.h"
#include "TimeManager.h"
#include "Input.h"
#include "GrassField.h"
#include "SceneManager.h"
#include "AudioPlayer.h"
#include "CoinManager.h"
#include "EnemyManager.h"
#include "EnvironmentPropManager.h"
#include "WeatherEffectManager.h"
#include "FoliageField.h"
#include "GameUI.h"
#include "Ground.h"
#include "Environment.h"
#include "TreeField.h"
#include "Player.h"
#include "PlayerWeapon.h"
#include "Tutorial.h"
#include "TitleMenu.h"
#include "PauseMenuHint.h"
#include "PauseMenu.h"

using namespace FS;

PlayScene::PlayScene(Engine* engine)
    : BaseScene(engine, SceneName::Play)
{
    // ゲームオブジェクトの生成・登録
    objectManager_.Create<Environment>(engine_);
    auto ground = objectManager_.Create<Ground>(engine_);
    Terrain* terrain = ground->GetTerrain();
    objectManager_.Create<GrassField>(engine_, terrain);
    auto treeField = objectManager_.Create<TreeField>(engine_, terrain);

    auto player = objectManager_.Create<Player>(engine_, camera_.get(), terrain, treeField);
    player_ = player;
    followCamera_ = std::make_unique<FollowCamera>(engine_, &player->GetTransform(), terrain);
    player->SetFollowCamera(followCamera_.get());
    titleCamera_ = std::make_unique<TitleCamera>(engine_, followCamera_.get());

    objectManager_.Create<WeatherEffectManager>(engine_, camera_.get(), cameraManager_.get(), player, terrain);
    auto coinManager = objectManager_.Create<CoinManager>(engine_, player, terrain);
    enemyManager_ = objectManager_.Create<EnemyManager>(engine_, "GameEnemy", terrain, coinManager);
    objectManager_.Create<EnvironmentPropManager>(engine_, "EnvironmentProps");
    objectManager_.Create<FoliageField>(engine_, terrain);
    gameUI_ = objectManager_.Create<GameUI>(engine_);
    tutorial_ = objectManager_.Create<Tutorial>(engine_, player);
    titleMenu_ = objectManager_.Create<TitleMenu>(engine_);
    objectManager_.Create<PauseMenuHint>(engine_);
    pauseMenu_ = objectManager_.Create<PauseMenu>(engine_);
}

void PlayScene::OnInitialize()
{
    followCamera_->Initialize();
    titleCamera_->Initialize();
    binder_->Bind("TimeLimit", &timeLimit_, 300.0f, 1.0f, 1.0f, 3600.0f);

    EnterTitle();
}

void PlayScene::OnUpdate()
{
    UpdatePauseMenu();

    switch (phase_)
    {
    case Phase::Title:
        if (titleMenu_->HasStarted())
        {
            titleCamera_->StartTransition(camera_.get());
            ChangePhase(Phase::CameraMove);
        }
        break;

    case Phase::CameraMove:
        if (titleCamera_->IsTransitionFinished())
        {
            StartPlaying();
        }
        break;

    case Phase::Playing:
        remainingTime_ -= TimeManager::GetInstance()->GetDeltaTime();
        if (remainingTime_ <= 0.0f)
        {
            ReturnToTitle();
        }
        break;
    }

    gameUI_->SetRemainingTime(remainingTime_);
}

void PlayScene::UpdatePauseMenu()
{
    pauseMenu_->SetCanOpen(phase_ != Phase::CameraMove);
    pauseMenu_->SetCanReturnToTitle(phase_ == Phase::Playing);

    if (std::optional<PauseMenu::Item> item = pauseMenu_->TakeSelectedItem())
    {
        switch (*item)
        {
        case PauseMenu::Item::Resume:
            break;
        case PauseMenu::Item::ToggleEditor:
            ImGuiManager::ToggleGui();
            break;
        case PauseMenu::Item::ReturnToTitle:
            ReturnToTitle();
            break;
        case PauseMenu::Item::QuitGame:
            engine_->RequestQuit();
            break;
        }
    }

    const bool isMenuOpen = pauseMenu_->IsOpen();
    if (isMenuOpen == isPaused_)
    {
        return;
    }
    isPaused_ = isMenuOpen;

    TimeManager* timeManager = TimeManager::GetInstance();
    if (isPaused_)
    {
        timeScaleBeforePause_ = timeManager->GetTimeScale();
        timeManager->SetTimeScale(0.0f);
    }
    else
    {
        timeManager->SetTimeScale(timeScaleBeforePause_);
    }
    ApplyPhaseSettings();
}

void PlayScene::ChangePhase(Phase phase)
{
    phase_ = phase;
    ApplyPhaseSettings();
}

void PlayScene::ApplyPhaseSettings()
{
    const bool isPlaying = phase_ == Phase::Playing;

    gameUI_->SetGameplayVisible(isPlaying);
    player_->SetControllable(isPlaying && !isPaused_);
    titleMenu_->SetInputEnabled(!isPaused_);

    if (isPaused_)
    {
        // ポーズ中はカメラを止める
        cameraManager_->ChangeController(nullptr);
    }
    else if (isPlaying)
    {
        cameraManager_->ChangeController(followCamera_.get());
    }
    else
    {
        // タイトル中と、タイトルからプレイヤーの後ろへ移る間
        cameraManager_->ChangeController(titleCamera_.get());
    }
}

void PlayScene::StartPlaying()
{
    tutorial_->Begin();
    remainingTime_ = timeLimit_;
    ChangePhase(Phase::Playing);
}

void PlayScene::EnterTitle()
{
    titleCamera_->CancelTransition();
    titleMenu_->Restart();
    remainingTime_ = timeLimit_;
    ChangePhase(Phase::Title);
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

void PlayScene::OnDebugGui()
{
    ImGui::Begin("タイトル");
    ImGui::SeparatorText("カメラ");
    titleCamera_->DrawProperties();

    ImGui::SeparatorText("ゲーム");
    binder_->Draw("TimeLimit", "制限時間 (秒)");
    ImGui::Text("残り: %.1f 秒", remainingTime_);

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
}

void PlayScene::OnFinalize()
{
    // ポーズ中にシーンを切り替えても時間が止まったままにならないように
    if (isPaused_)
    {
        TimeManager::GetInstance()->SetTimeScale(timeScaleBeforePause_);
        isPaused_ = false;
    }
}
