#include "SampleSceneHori.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "ImGuiManager.h"
#include "ModelLoader.h"
#include "GlobalVariables.h"
#include "AnimationHandle.h"
#include "TimeManager.h"
#include "Input.h"

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
    enemyManager_ = std::make_unique<EnemyManager>(engine_, camera_, player_, &objectManager_);
    particleSystemWrapper_ = std::make_unique<ParticleSystemWrapper>(engine_, camera_);

    // 作成したゲームオブジェクトを管理クラスに登録
    objectManager_.AddObject(std::move(player));
    objectManager_.AddObject(std::move(particleSystemWrapper_));

}

void SampleSceneHori::Initialize()
{
    // ゲームオブジェクトの一括初期化
    objectManager_.Initialize();

    followCamera_.Initialize(camera_, player_);

}

void SampleSceneHori::Update()
{
    // ゲームオブジェクトの調整項目を一括更新
    objectManager_.ApplyGlobalVariables();

    HandleCollisions();

    enemyManager_->Update();
    // ゲームオブジェクトの一括更新
    objectManager_.Update();

    followCamera_.Update();

    objectManager_.SaveGlobalVariables();
}

void SampleSceneHori::HandleCollisions()
{
    // 衝突マネージャのリストをクリアする
    collisionManager_->ClearColliders();

    // ObjectManager に「全オブジェクトを登録して」と依頼
    objectManager_.AddAllCollidersToManager(collisionManager_.get());

    // プレイヤーが持つ武器の弾を登録
    player_->AddWeaponColliders(collisionManager_.get());
   
    // 衝突マネージャの当たり判定処理を呼び出す
    collisionManager_->CheckAllCollisions();
}


void SampleSceneHori::Draw()
{
    // ゲームオブジェクトの一括描画
    objectManager_.Draw();
}

void SampleSceneHori::DebugDraw()
{
    ImGui::Begin("ホリシーン");
   
    ImGui::End();
    // ゲームオブジェクトの一括デバッグ描画
    objectManager_.DebugDraw();

    followCamera_.DebugDraw();
}

void SampleSceneHori::Finalize()
{
}
