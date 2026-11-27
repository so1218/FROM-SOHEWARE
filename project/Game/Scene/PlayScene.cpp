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
#include "GameOverScreen.h"
#include "ResultScreen.h"
#include "ObjectiveBanner.h"
#include "EnvironmentManager.h"
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
    coinManager_ = objectManager_.Create<CoinManager>(engine_, player, terrain);
    enemyManager_ = objectManager_.Create<EnemyManager>(engine_, "GameEnemy", terrain, treeField, coinManager_);
    objectManager_.Create<EnvironmentPropManager>(engine_, stage.GetGroupName("EnvironmentProps"));
    objectManager_.Create<FoliageField>(engine_, terrain, stage.GetGroupName("FoliageField"));
    gameUI_ = objectManager_.Create<GameUI>(engine_);
    tutorial_ = objectManager_.Create<Tutorial>(engine_, player);
    titleMenu_ = objectManager_.Create<TitleMenu>(engine_);
    objectManager_.Create<PauseMenuHint>(engine_);
    pauseMenu_ = objectManager_.Create<PauseMenu>(engine_);
    gameOverScreen_ = objectManager_.Create<GameOverScreen>(engine_);
    resultScreen_ = objectManager_.Create<ResultScreen>(engine_);
    objectiveBanner_ = objectManager_.Create<ObjectiveBanner>(engine_);

    flowBinder_ = std::make_unique<PropertyBinder>(engine_, "GameFlow");
}

void PlayScene::OnInitialize()
{
    flowBinder_->Bind("StartHour", &startHour_, 17.0f, 0.1f, 0.0f, 24.0f);
    flowBinder_->Bind("StormHour", &stormHour_, 19.0f, 0.1f, 0.0f, 24.0f);
    flowBinder_->Bind("EndHour", &endHour_, 20.0f, 0.1f, 0.0f, 24.0f);
    flowBinder_->Bind("CalmDuration", &calmDuration_, 120.0f, 1.0f, 1.0f, 3600.0f);
    flowBinder_->Bind("StormDuration", &stormDuration_, 60.0f, 1.0f, 1.0f, 3600.0f);
    flowBinder_->Bind("StormLeadTime", &stormLeadTime_, 10.0f, 0.1f, 0.0f, 60.0f);
    flowBinder_->Bind("TutorialEndDelay", &tutorialEndDelay_, 2.0f, 0.1f, 0.0f, 10.0f);

    followCamera_->Initialize();
    titleCamera_->Initialize();
    EnterTitle();

    // タイトルでステージを切り替えて作り直したときは、タイトルの黒い画面を省く
    if (StageCatalog::TakeJustSwitched())
    {
        titleMenu_->SkipFadeIn();
    }
}

void PlayScene::OnUpdate()
{
    UpdatePauseMenu();
    // 本番の時計が動いている間は、左上に目的と制限時間を出しておく
    gameUI_->SetObjectiveVisible(isClockRunning_);

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
        if (player_->IsDead())
        {
            gameOverScreen_->Begin();
            followCamera_->SetDeathFocus(true);
            ChangePhase(Phase::GameOver);
            break;
        }
        // チュートリアル中は時計を止めておき、終えたら少し待って本番の準備に移る
        if (!isClockRunning_)
        {
            if (tutorial_->IsCompleted())
            {
                // 長押しで飛ばしたときは、待たずにすぐ暗転する
                tutorialEndTimer_ += TimeManager::GetInstance()->GetDeltaTime();
                if (tutorialEndTimer_ >= tutorialEndDelay_ || tutorial_->IsSkipped())
                {
                    BeginObjective(true);
                }
            }
            break;
        }
        UpdateGameClock();
        if (playTimer_ >= GetTotalPlayTime())
        {
            resultScreen_->Begin(player_->GetMoney());
            ChangePhase(Phase::Result);
        }
        break;

    case Phase::GameOver:
        if (gameOverScreen_->TakeRestartRequest())
        {
            RestartFromSpawn();
            BeginPlayingOrObjective();
        }
        break;

    case Phase::Intro:
        // 真っ暗な間に、敵・お金・燃えた草ごと最初の状態に戻す
        if (objectiveBanner_->TakeResetRequest())
        {
            RestartFromSpawn();
        }
        if (objectiveBanner_->TakeFinished())
        {
            isClockRunning_ = true;
            ChangePhase(Phase::Playing);
            gameUI_->FlashClock();
        }
        break;

    case Phase::Result:
        if (std::optional<ResultScreen::Choice> choice = resultScreen_->TakeRequest())
        {
            if (*choice == ResultScreen::Choice::Retry)
            {
                RestartFromSpawn();
                BeginPlayingOrObjective();
            }
            else
            {
                ReturnToTitle();
            }
        }
        break;
    }
}

void PlayScene::ResetGameClock()
{
    playTimer_ = 0.0f;
    isClockRunning_ = false;
    isStormRequested_ = false;
    tutorialEndTimer_ = 0.0f;

    EnvironmentManager* environment = EnvironmentManager::GetInstance();
    environment->SetTimeFlowEnabled(false);
    environment->SetTimeOfDay(startHour_);
    environment->SetWeatherImmediately(WeatherState::Sunny);
}

void PlayScene::BeginObjective(bool withFade)
{
    objectiveBanner_->Begin(withFade, !hasShownObjective_, static_cast<int>(endHour_), GetTotalPlayTime());
    gameUI_->SetObjective(static_cast<int>(startHour_), static_cast<int>(stormHour_), static_cast<int>(endHour_));
    hasShownObjective_ = true;
    ChangePhase(Phase::Intro);
}

void PlayScene::BeginPlayingOrObjective()
{
    if (tutorial_->IsCompleted())
    {
        BeginObjective(false);
    }
    else
    {
        ChangePhase(Phase::Playing);
    }
}

void PlayScene::UpdateGameClock()
{
    // ポーズ中は時間の速さが 0 なので、時計も止まる
    playTimer_ += TimeManager::GetInstance()->GetDeltaTime();

    EnvironmentManager* environment = EnvironmentManager::GetInstance();
    environment->SetTimeOfDay(CalculateHour(playTimer_));

    if (!isStormRequested_ && playTimer_ >= calmDuration_ - stormLeadTime_)
    {
        environment->RequestWeatherChange(WeatherState::Thunderstorm);
        isStormRequested_ = true;
    }
}

float PlayScene::CalculateHour(float elapsedTime) const
{
    if (elapsedTime < calmDuration_)
    {
        return startHour_ + (stormHour_ - startHour_) * (elapsedTime / calmDuration_);
    }
    const float stormRatio = std::min((elapsedTime - calmDuration_) / stormDuration_, 1.0f);
    return stormHour_ + (endHour_ - stormHour_) * stormRatio;
}

void PlayScene::UpdatePauseMenu()
{
    // 倒れている間は時間の流れを演出で変えているので、ポーズさせない
    pauseMenu_->SetCanOpen(phase_ != Phase::CameraMove && phase_ != Phase::GameOver && phase_ != Phase::Result && phase_ != Phase::Intro);
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
    // 目的を見せている間とリザルトの間は敵に襲わせない。デバッグで切り替えていた状態は戻す
    if (IsEnemyFreePhase(phase) && !IsEnemyFreePhase(phase_))
    {
        wasIgnoredByEnemies_ = player_->IsIgnoredByEnemies();
        player_->SetIgnoredByEnemies(true);
    }
    else if (!IsEnemyFreePhase(phase) && IsEnemyFreePhase(phase_))
    {
        player_->SetIgnoredByEnemies(wasIgnoredByEnemies_);
    }

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
    else if (isPlaying || phase_ == Phase::GameOver || phase_ == Phase::Result || phase_ == Phase::Intro)
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
    BeginPlayingOrObjective();
}

void PlayScene::EnterTitle()
{
    titleCamera_->CancelTransition();
    titleMenu_->Restart();
    // タイトルでは時刻を止めて、遊び始めの時刻と晴れにしておく
    ResetGameClock();
    ChangePhase(Phase::Title);
}

void PlayScene::RestartFromSpawn()
{
    player_->ResetToSpawn();
    enemyManager_->ResetEnemies();
    coinManager_->Clear();
    engine_->GetRendererManager()->ResetGrassFire();
    // 倒れた場所からカメラが追いかけてこないよう、すぐ後ろに置き直す
    followCamera_->SetDeathFocus(false);
    followCamera_->PlaceBehindTarget();
    followCamera_->Reset(camera_.get());
    // 遊び直すときは時計も最初から
    ResetGameClock();
}

void PlayScene::ReturnToTitle()
{
    gameOverScreen_->Cancel();
    resultScreen_->Cancel();
    objectiveBanner_->Cancel();
    followCamera_->SetDeathFocus(false);
    player_->ResetToSpawn();
    enemyManager_->ResetEnemies();
    coinManager_->Clear();
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

    ImGui::Begin("ゲームの流れ");
    flowBinder_->Draw("StartHour", "始まりの時刻");
    flowBinder_->Draw("StormHour", "雷雨になる時刻");
    flowBinder_->Draw("EndHour", "終わりの時刻");
    flowBinder_->Draw("CalmDuration", "晴れの間の長さ (秒)");
    flowBinder_->Draw("StormDuration", "雷雨の間の長さ (秒)");
    flowBinder_->Draw("StormLeadTime", "雷雨に変え始める早さ (秒前)");
    flowBinder_->Draw("TutorialEndDelay", "チュートリアルを終えてから暗転するまで (秒)");

    ImGui::SeparatorText("確認用");
    if (isClockRunning_)
    {
        ImGui::Text("経過 %.1f / %.1f 秒  (%.2f 時)", playTimer_, GetTotalPlayTime(), CalculateHour(playTimer_));
    }
    else
    {
        ImGui::TextUnformatted("時計は止まっています (タイトル・チュートリアル中・目的の表示中)");
    }
    if (phase_ == Phase::Playing && isClockRunning_)
    {
        if (ImGui::Button("雷雨の直前へ"))
        {
            playTimer_ = std::max(playTimer_, calmDuration_ - stormLeadTime_ - 1.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("終わりの直前へ"))
        {
            playTimer_ = std::max(playTimer_, GetTotalPlayTime() - 3.0f);
        }
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

    // 倒れた直後のスローを次のシーンに残さない
    gameOverScreen_->Cancel();

    // 時刻の自動の進みを止めたままにしない
    EnvironmentManager::GetInstance()->SetTimeFlowEnabled(true);

    // 隠れているときのビネットと周りの音は毎フレーム渡す値なので、次のシーンに残さない
    engine_->GetPostEffectManager()->SetOverlayVignette(0.0f, { 0.0f, 0.0f, 0.0f });
    AudioPlayer::GetInstance().SetWorldVolume(1.0f);
    AudioPlayer::GetInstance().SetWorldMuffle(0.0f);
}
