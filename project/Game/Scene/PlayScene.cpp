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

PlayScene::PlayScene(Engine* engine, Camera* camera)
{
    // ポインタを保存
    engine_ = engine;
    camera_ = camera;

    // オブジェクトを生成
    player_ = std::make_unique<Player>(engine_, camera_);
    enemy_ = std::make_unique<Enemy>(engine_, camera_);
    collisionManager_ = std::make_unique<CollisionManager>();
    emitter_ = std::make_unique<ParticleEmitter>();

    // エミッターを初期化
    emitter_->Initialize(ParticleType::Key, { 0.0f, 1.0f, 0.0f }, 0.3f, 4.0f, 20);

    // パーティクルシステムにエミッターを登録
    engine_->particleSystem_->AddEmitter(emitter_.get());
}

PlayScene::~PlayScene()
{

}

void PlayScene::Initialize()
{
    // 初期化
    player_->Initialize();
    camera_->Initialize();
}

void PlayScene::Update()
{
    // 衝突処理の実行
    HandleCollisions();
    
    // プレイヤーの更新処理
    player_->Update();
    // パーティクルシステムを更新
    engine_->particleSystem_->Update();

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
    /*player_->Draw();*/
    engine_->SetBlendMode(BlendMode::kBlendModeAdd);
    engine_->DrawParticles(*camera_);
}

void PlayScene::DebugDraw()
{
    ImGui::Begin("プレイシーン");

    ImGui::End();

	player_->DebugDraw();
    enemy_->DebugDraw();
    engine_->particleSystem_->ShowEditor();
}

void PlayScene::Finalize()
{

}

