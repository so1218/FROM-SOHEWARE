#include "pch.h"
#include "PlayScene.h"
#include "GameDefine.h"
#include "ImGuiManager.h"
#include "TimeManager.h"
#include "Input.h"
#include "GrassField.h"
#include "GrassFireEffects.h"
#include "FireflyField.h"
#include "DistantMountains.h"
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
#include "PostEffectManager.h"
#include "RendererManager.h"
#include "SceneManager.h"
#include "StageCatalog.h"

using namespace FS;

PlayScene::PlayScene(Engine* engine)
    : BaseScene(engine, SceneName::Play, StageCatalog::GetSelectedStage().GetGroupName("PlayScene"))
{
    // フィールド・時間と天気・空・ライトとポストエフェクトは、選んでいるステージの設定を使う
    const StageDefinition& stage = StageCatalog::GetSelectedStage();

    // ゲームオブジェクトの生成・登録
    objectManager_.Create<Environment>(engine_, stage.GetGroupName("Environment"), stage.GetGroupName("Skydome"));
    auto ground = objectManager_.Create<Ground>(engine_, stage.GetGroupName("Ground"));
    Terrain* terrain = ground->GetTerrain();
    auto distantMountains = objectManager_.Create<DistantMountains>(engine_, terrain, stage.GetGroupName("DistantMountains"));
    auto grassField = objectManager_.Create<GrassField>(engine_, terrain, stage.GetGroupName("GrassField"));
    objectManager_.Create<GrassFireEffects>(engine_, terrain);
    auto treeField = objectManager_.Create<TreeField>(engine_, terrain, stage.GetGroupName("TreeField"));
    treeField->SetDistantMountains(distantMountains);

    auto player = objectManager_.Create<Player>(engine_, camera_.get(), terrain, treeField);
    player_ = player;
    player->SetGrassField(grassField);
    objectManager_.Create<FireflyField>(engine_, terrain, &player->GetTransform(), stage.GetGroupName("FireflyField"));
    followCamera_ = std::make_unique<FollowCamera>(engine_, &player->GetTransform(), terrain);
    player->SetFollowCamera(followCamera_.get());
    titleCamera_ = std::make_unique<TitleCamera>(engine_, followCamera_.get(), stage.GetGroupName("TitleCamera"));

    objectManager_.Create<WeatherEffectManager>(engine_, camera_.get(), cameraManager_.get(), player, terrain);
    auto coinManager = objectManager_.Create<CoinManager>(engine_, player, terrain);
    enemyManager_ = objectManager_.Create<EnemyManager>(engine_, "GameEnemy", terrain, coinManager);
    objectManager_.Create<EnvironmentPropManager>(engine_, stage.GetGroupName("EnvironmentProps"));
    objectManager_.Create<FoliageField>(engine_, terrain, stage.GetGroupName("FoliageField"));
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

    // タイトルでステージを切り替えて作り直したときは、画面のフェードで十分なので、タイトルの黒い画面を省く
    if (StageCatalog::TakeJustSwitched())
    {
        titleMenu_->SkipFadeIn();
    }
}

void PlayScene::OnUpdate()
{
    UpdatePauseMenu();

    switch (phase_)
    {
    case Phase::Title:
        // ステージを切り替えたら、そのステージの設定でシーンを作り直す
        if (titleMenu_->TakeStageChangeRequest())
        {
            sceneManager_->RequestSceneReload();
            break;
        }
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
    // 次に遊ぶときは、燃える前の草原から始める
    engine_->GetRendererManager()->ResetGrassFire();
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

    // 隠れているときのビネットは毎フレーム渡す値なので、次のシーンに残さない
    engine_->GetPostEffectManager()->SetOverlayVignette(0.0f, { 0.0f, 0.0f, 0.0f });
}
