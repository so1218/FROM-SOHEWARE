#include "SampleSceneHori.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "ImGuiManager.h"
#include "ModelLoader.h"
#include "GlobalVariables.h"
#include "AnimationHandle.h"
#include "TimeManager.h"
#include "Input.h"
#include "Grid.h"
#include "PlayerUI.h"
#include "SceneManager.h"
#include "AudioPlayer.h"
#include "AudioHandle.h"

using namespace FromEngine;

SampleSceneHori::SampleSceneHori(Engine* engine, Camera* camera)
{
    // ポインタを保持
    engine_ = engine;
    camera_ = camera;

    collisionManager_ = std::make_unique<CollisionManager>();

    // インスタンスを作成
    auto player = std::make_unique<Player>(engine_, camera_);
    player_ = player.get();
    player_->AddWeapon(WeaponType::Axe);
    player_->AddWeapon(WeaponType::Knife);
    auto playerUI = std::make_unique<PlayerUI>(engine_, player_);
    auto followCamera = std::make_unique<FollowCamera>(camera_, player_);
    followCamera_ = followCamera.get();
    player_->SetFollowCamera(followCamera_);
    enemyManager_ = std::make_unique<EnemyManager>(engine_, camera_, player_, &objectManager_);
    particleSystemWrapper_ = std::make_unique<ParticleSystemWrapper>(engine_, camera_);

    skybox_ = std::make_unique<Skybox>(engine_, camera_);
    uint32_t cubemapHandle = TextureHandle::Get(TextureID::skyboxCubemap);
    skybox_->SetCubeTextureHandle(cubemapHandle);
    auto grid = std::make_unique<Grid>(engine_, camera_);

    // タイマーの生成
    auto gameTimer = std::make_unique<GameTimer>(engine_);
    gameTimer_ = gameTimer.get();

    levelUpManager_ = std::make_unique<LevelUpManager>();
    levelUpUI_ = std::make_unique<LevelUpUI>(engine_);

    // 作成したゲームオブジェクトを管理クラスに登録
    objectManager_.AddObject(std::move(player));
    objectManager_.AddObject(std::move(playerUI));
    objectManager_.AddObject(std::move(particleSystemWrapper_));
    objectManager_.AddObject(std::move(followCamera));
    //objectManager_.AddObject(std::move(grid));
    objectManager_.AddObject(std::move(gameTimer));

}

void SampleSceneHori::Initialize()
{
    // ゲームオブジェクトの一括初期化
    objectManager_.Initialize();

    playerWalkEmitter_ = engine_->particleSystem_->CreateEmitter("PlayerWalk");
    player_->SetWalkEmitter(playerWalkEmitter_.get());
    playerWalkEmitter_->SetTargetToFollow(&player_->modelPlayer_->GetTransform());
    engine_->particleSystem_->AddEmitter(std::move(playerWalkEmitter_));
    playerLevelUpEmitter_ = engine_->particleSystem_->CreateEmitter("PlayerLevelUp");
    player_->SetLevelUpEmitter(playerLevelUpEmitter_.get());
    playerLevelUpEmitter_->SetTargetToFollow(&player_->modelPlayer_->GetTransform());
    engine_->particleSystem_->AddEmitter(std::move(playerLevelUpEmitter_));
    playerDamagedEmitter_ = engine_->particleSystem_->CreateEmitter("PlayerDamaged");
    player_->SetDamagedEmitter(playerDamagedEmitter_.get());
    playerDamagedEmitter_->SetTargetToFollow(&player_->modelPlayer_->GetTransform());
    engine_->particleSystem_->AddEmitter(std::move(playerDamagedEmitter_));
    playerGetExpEmitter_ = engine_->particleSystem_->CreateEmitter("playerGetExp");
    player_->SetGetExpEmitter(playerGetExpEmitter_.get());
    playerGetExpEmitter_->SetTargetToFollow(&player_->modelPlayer_->GetTransform());
    engine_->particleSystem_->AddEmitter(std::move(playerGetExpEmitter_));
    sceneEmitter_ = engine_->particleSystem_->CreateEmitter("scene");
    sceneEmitter_->SetTargetToFollow(&player_->modelPlayer_->GetTransform());
    engine_->particleSystem_->AddEmitter(std::move(sceneEmitter_));

    // ライトの設定
    engine_->lightManager_->GetDirectionalLightData()[0].enable = true;
    engine_->lightManager_->GetDirectionalLightData()[0].direction = { -0.05f,-1.45f,1.4f };
    engine_->lightManager_->GetDirectionalLightData()[0].intensity = 0.4f;
    engine_->materialManager_->GetMaterialSettings().enableLighting = true;
    engine_->materialManager_->GetMaterialSettings().lightMode = 1 ;

    //engine_->postEffectManager_->GetPostEffectData()->modeFlags[0] |= VIGNETTE;
    //engine_->postEffectManager_->GetPostEffectData()->vignetteAmount = 1.29f;
    //engine_->postEffectManager_->GetPostEffectData()->vignetteRadius = 0.029f;
    //engine_->postEffectManager_->GetPostEffectData()->vignetteSoftness = 0.723f;
    //engine_->postEffectManager_->GetPostEffectData()->vignetteEllipseScale = { 1.05f,0.95f };
    //engine_->postEffectManager_->GetPostEffectData()->vignetteColor = { 6.0f / 255.0f,42.0f / 255.0f,72.0f / 255.0f };
    //engine_->postEffectManager_->GetPostEffectData()->modeFlags[0] |= COLOR_TINT;
    //engine_->postEffectManager_->GetPostEffectData()->tintColor = { 130.0f / 255.0f,255.0f / 255.0f,241.0f / 255.0f };
    //engine_->postEffectManager_->GetPostEffectData()->tintMulColorAmount = 0.015f;
    //engine_->postEffectManager_->GetPostEffectData()->tintAddColorAmount = 0.075f;
    //engine_->postEffectManager_->GetPostEffectData()->tintScreenColorAmount = 0.25f;
    //engine_->postEffectManager_->GetCombineSettings()->enableFog = true;
    //engine_->postEffectManager_->GetCombineSettings()->fogEnd = 100.0f;
    //engine_->postEffectManager_->GetCombineSettings()->fogStart = 20.0f;
    //engine_->postEffectManager_->GetCombineSettings()->fogEnd = 100.0f;
    //engine_->postEffectManager_->GetCombineSettings()->fogColor = { 86.0f / 255.0f,175.0f / 255.0f,254.0f / 255.0f };

    levelUpUI_->Initialize();

    sceneState_ = SceneState::Playing;

    // 制限時間を設定
    gameTimer_->Initialize(2.0f);

    objectManager_.ClearEnemies();

    enemyManager_->Reset();

    AudioPlayer::GetInstance().StopUnique(AudioHandle::Get(AudioID::clearSceneBGM));
    AudioPlayer::GetInstance().StopUnique(AudioHandle::Get(AudioID::titleSceneBGM));
}

void SampleSceneHori::Update()
{
    AudioPlayer::GetInstance().PlayUnique(AudioHandle::Get(AudioID::playSceneBGM), true, 20);

    switch (sceneState_)
    {
    case SceneState::Playing:
        UpdatePlaying();
        break;

    case SceneState::LevelUpSelection:
        UpdateLevelUpSelection();
        break;
    }
}

void SampleSceneHori::UpdatePlaying()
{
    HandleCollisions();

    // ゲームオブジェクトの更新
    enemyManager_->Update();
    objectManager_.Update();

    // プレイヤーがレベルアップして待機状態になったかチェック
    if (player_->IsWaitingForUpgrade())
    {
        // 抽選
        auto candidates = levelUpManager_->PickUpgrades(player_);

        // UI表示
        if (!candidates.empty())
        {
            // UIに候補を渡して起動
            levelUpUI_->Activate(candidates);

            // 状態をレベルアップ画面へ移行
            sceneState_ = SceneState::LevelUpSelection;
        }
        else
        {
            // 候補がない場合はそのまま継続（またはHP回復など代替処理）
            player_->FinishUpgrade();
        }
    }

    // ゲーム終了判定
    if (player_->IsEnd())
    {
       /* sceneManager_->RequestSceneChange(SceneID::Sample);*/
    }
    // タイムアップ
    if (gameTimer_->IsTimeUp())
    {
       /* sceneManager_->RequestSceneChange(SceneID::Play);*/
        AudioPlayer::GetInstance().PlayUnique(AudioHandle::Get(AudioID::clearSE), false, 100);
    }
}

// レベルアップ選択画面中の更新処理
void SampleSceneHori::UpdateLevelUpSelection()
{
    // UIだけ更新する
    levelUpUI_->Update();

    // 決定されたかチェック
    if (levelUpUI_->IsDecided())
    {
        // 選んだ情報を取得
        UpgradeInfo selectedUpgrade = levelUpUI_->GetDecision();

        // プレイヤーに適用
        player_->ApplyUpgrade(selectedUpgrade);

        // プレイヤーの待機フラグを下ろす
        player_->FinishUpgrade();

        // ゲームプレイ状態に戻る
        sceneState_ = SceneState::Playing;
    }
}

void SampleSceneHori::HandleCollisions()
{
    // 衝突マネージャのリストをクリアする
    collisionManager_->ClearColliders();

    // ObjectManager に全オブジェクトを登録してと依頼
    objectManager_.AddAllCollidersToManager(collisionManager_.get());

    // プレイヤーが持つ武器の弾を登録
    player_->AddWeaponColliders(collisionManager_.get());
   
    // 衝突マネージャの当たり判定処理を呼び出す
    collisionManager_->CheckAllCollisions();
}


void SampleSceneHori::Draw()
{
 /*   skybox_->Draw();*/
    // ゲームオブジェクトの一括描画
    objectManager_.Draw();

    // レベルアップ選択中なら、その上にUIを描画
    if (sceneState_ == SceneState::LevelUpSelection)
    {
        // 半透明の黒背景を描画したい
        levelUpUI_->Draw();
    }
}

void SampleSceneHori::DebugDraw()
{
    ImGui::Begin("ホリシーン");
   
    ImGui::End();
    // ゲームオブジェクトの一括デバッグ描画
    objectManager_.DebugDraw();
    levelUpUI_->DebugDraw();
}

void SampleSceneHori::Finalize()
{
}
