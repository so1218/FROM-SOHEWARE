#pragma once

#include "BaseScene.h"
#include "Player.h"
#include "CollisionManager.h"
#include "ParticleEmitter.h"
#include "GameObjectManager.h"
#include "Enemy.h"
#include "FollowCamera.h"
#include "ParticleSystemWrapper.h"
#include "GameObjectManager.h"

class PlayScene : public BaseScene
{
public:
    PlayScene(Engine* engine, Camera* camera);

    ~PlayScene();

    // 初期化処理
    void Initialize() override;

    // 更新処理
    void Update() override;

    // 描画処理
    void Draw() override;

    // デバッグ描画処理
    void DebugDraw() override;

    // 終了処理
    void Finalize() override;

    // 衝突に関する処理をまとめる関数
    void HandleCollisions();

    GameObjectManager objectManager_;

     // メンバー変数
    Engine* engine_;
    Camera* camera_;

    std::unique_ptr<Player> player_;
    std::unique_ptr<Enemy> enemy_;
    std::unique_ptr<CollisionManager> collisionManager_;
    std::unique_ptr<ParticleSystemWrapper> particleSystemWrapper_;
    std::unique_ptr<ParticleEmitter> emitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> newEmitter_ = nullptr;
};

