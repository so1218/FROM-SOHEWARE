#pragma once
#include "Engine.h"
#include "Camera.h"
#include "MaterialManager.h"
#include "Model.h"
#include "Sprite.h"
#include "BaseScene.h"
#include "GameObjectManager.h"
#include "CollisionManager.h"
#include "Player.h"
#include "Enemy.h"
#include "ShakeEffect.h"
#include "AnimationModel.h"
#include "FollowCamera.h"
#include "EnemyManager.h"
#include "ParticleSystemWrapper.h"

class SampleSceneHori : public BaseScene
{
public:
	SampleSceneHori(Engine* engine, Camera* camera);

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
private:
    // メンバー変数
    Engine* engine_;
    Camera* camera_;
    GameObjectManager objectManager_;
    std::unique_ptr<CollisionManager> collisionManager_;
    FollowCamera followCamera_;
    std::unique_ptr<EnemyManager> enemyManager_;
    std::unique_ptr<ParticleSystemWrapper> particleSystemWrapper_;

    Player* player_ = nullptr;
    Enemy* enemy_ = nullptr;

};

