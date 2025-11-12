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

PlayScene::PlayScene(Engine* engine, Camera* camera)
{
    // ポインタを保存
    engine_ = engine;
    camera_ = camera;

    // オブジェクトを生成
    player_ = std::make_unique<Player>(engine_, camera_);
    followCamera_.Initialize(camera_, player_.get());

    collisionManager_ = std::make_unique<CollisionManager>();
    particleSystemWrapper_ = std::make_unique<ParticleSystemWrapper>(engine_, camera_);
    auto grid = std::make_unique<Grid>(engine_, camera_, std::move(ModelHandle::Get(ModelID::field)));

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


    emitter_ = engine_->particleSystem_->CreateEmitter("chest");
    newEmitter_ = engine_->particleSystem_->CreateEmitter("doar");

    engine_->particleSystem_->AddEmitter(std::move(emitter_));
    engine_->particleSystem_->AddEmitter(std::move(newEmitter_));

    // ゲームオブジェクトの一括初期化
    objectManager_.Initialize();
}

void PlayScene::Update()
{
    // ゲームオブジェクトの調整項目を一括更新
    objectManager_.ApplyGlobalVariables();

    HandleCollisions();

    // ゲームオブジェクトの一括更新
    objectManager_.Update();

    followCamera_.Update();

    objectManager_.SaveGlobalVariables();
    
    // プレイヤーの更新処理
    player_->Update();
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
	objectManager_.Draw();
}

void PlayScene::DebugDraw()
{
    ImGui::Begin("プレイシーン");

    ImGui::End();

	objectManager_.DebugDraw();

	player_->DebugDraw();
  /*  enemy_->DebugDraw();*/
    followCamera_.DebugDraw();

}

void PlayScene::Finalize()
{

}

