#include "PlayScene.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "MediaAudioDecoder.h"
#include "Collision.h"
#include "ImGuiManager.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "AudioHandle.h"
#include "AudioPlayer.h"
#include "TimeManager.h"
#include "MathUtils.h"
#include "ModelLoader.h"
#include "Input.h"
#include "Grid.h"

using namespace FromEngine;

PlayScene::PlayScene(Engine* engine, Camera* camera)
{
    // ポインタを保存
    engine_ = engine;
    camera_ = camera;

    // オブジェクトを生成
    player_ = std::make_unique<Player>(engine_, camera_);

    collisionManager_ = std::make_unique<CollisionManager>();
    particleSystemWrapper_ = std::make_unique<ParticleSystemWrapper>(engine_, camera_);
    auto grid = std::make_unique<Grid>(engine_, camera_);

    sprite_ = std::make_unique<Sprite>(engine_);
    spriteUse_ = std::make_unique<Sprite>(engine_);

    objectManager_.AddObject(std::move(grid));
    objectManager_.AddObject(std::move(particleSystemWrapper_));
}

PlayScene::~PlayScene()
{

}

void PlayScene::Initialize()
{
    // 初期化
    player_->Initialize();
    camera_->Initialize();

    hanabi1Emitter_ = engine_->particleSystem_->CreateEmitter("hanabi1");
    hanabi2Emitter_ = engine_->particleSystem_->CreateEmitter("hanabi2");
    hanabi3Emitter_ = engine_->particleSystem_->CreateEmitter("hanabi3");
    clearEmitter_ = engine_->particleSystem_->CreateEmitter("clear");

    engine_->particleSystem_->AddEmitter(std::move(hanabi1Emitter_));
    engine_->particleSystem_->AddEmitter(std::move(hanabi2Emitter_));
    engine_->particleSystem_->AddEmitter(std::move(hanabi3Emitter_));
    engine_->particleSystem_->AddEmitter(std::move(clearEmitter_));

    engine_->postEffectManager_->GetCombineSettings()->enableFog = true;
    engine_->postEffectManager_->GetCombineSettings()->fogEnd = 5000.0f;

    spritePos_ = { 640, 227 };
    sprite_->SetPosition(spritePos_);
    spriteSize_ = { 800.0f, 280.0f };
    sprite_->SetSize(spriteSize_);
    sprite_->SetAnchorPoint({ 0.5f, 0.5f });
    sprite_->SetTextureHandle(TextureHandle::Get(TextureID::clear));

    spritePosUse_ = { 640, 522 };
    spriteUse_->SetPosition(spritePosUse_);
    spriteSizeUse_ = { 800.0f, 131.0f };
    spriteUse_->SetSize(spriteSizeUse_);
    spriteUse_->SetAnchorPoint({ 0.5f, 0.5f });
    spriteUse_->SetTextureHandle(TextureHandle::Get(TextureID::pressSousa));

    // ゲームオブジェクトの一括初期化
    objectManager_.Initialize();

    AudioPlayer::GetInstance().StopUnique(AudioHandle::Get(AudioID::playSceneBGM));
    AudioPlayer::GetInstance().StopUnique(AudioHandle::Get(AudioID::titleSceneBGM));
}

void PlayScene::Update()
{
    AudioPlayer::GetInstance().PlayUnique(AudioHandle::Get(AudioID::clearSceneBGM), true, 20);

    HandleCollisions();

    // ゲームオブジェクトの一括更新
    objectManager_.Update();

    // プレイヤーの更新処理
    player_->Update();

    if (Input::GetInstance().IsKeyTriggered(DIK_SPACE) || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonA)
        || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonB))
    {
        // シーンマネージャーを通じてシーン切り替えをリクエスト
        sceneManager_->RequestSceneChange(SceneID::Title);
        AudioPlayer::GetInstance().PlayUnique(AudioHandle::Get(AudioID::dicision), false, 100);
    }
}

void PlayScene::HandleCollisions()
{
    // 衝突マネージャのリストをクリアする
    collisionManager_->ClearColliders();

    // プレイヤーと敵を登録
    collisionManager_->AddCollider(player_.get());

    // 衝突マネージャの当たり判定処理を呼び出す
    collisionManager_->CheckAllCollisions();
}

void PlayScene::Draw()
{
    sprite_->Draw();
    spriteUse_->Draw();
	objectManager_.Draw();
}

void PlayScene::DebugDraw()
{
    ImGui::Begin("プレイシーン");

    if (ImGui::DragFloat2("Sprite Pos", &spritePos_.x, 1.0f))
    {
        sprite_->SetPosition(spritePos_);
    }
    if (ImGui::DragFloat2("Sprite Size", &spriteSize_.x, 1.0f))
    {
        sprite_->SetSize(spriteSize_);
    }
    if (ImGui::DragFloat2("Sprite Pos Use", &spritePosUse_.x, 1.0f))
    {
        spriteUse_->SetPosition(spritePosUse_);
    }
    if (ImGui::DragFloat2("Sprite Size Use", &spriteSizeUse_.x, 1.0f))
    {
        spriteUse_->SetSize(spriteSizeUse_);
    }

    ImGui::End();

	objectManager_.DebugDraw();

	player_->DebugDraw();
  /*  enemy_->DebugDraw();*/

}

void PlayScene::Finalize()
{

}

