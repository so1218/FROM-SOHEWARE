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
#include "ParticleEmitter.h"
#include "Skybox.h"
#include "GameTimer.h"

class SampleSceneHori : public BaseScene
{
public:
	SampleSceneHori(Engine* engine, Camera* camera);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;
    void Finalize() override;

    // 衝突に関する処理をまとめる関数
    void HandleCollisions();
private:
    // メンバー変数
    Engine* engine_;
    Camera* camera_;
    GameObjectManager objectManager_;
    std::unique_ptr<CollisionManager> collisionManager_;
    std::unique_ptr<EnemyManager> enemyManager_;
    std::unique_ptr<ParticleSystemWrapper> particleSystemWrapper_;
    std::unique_ptr<ParticleEmitter> playerWalkEmitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> playerLevelUpEmitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> playerDamagedEmitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> playerGetExpEmitter_ = nullptr;

    Player* player_ = nullptr;
    Enemy* enemy_ = nullptr;
    FollowCamera* followCamera_ = nullptr;
    GameTimer* gameTimer_ = nullptr;

    std::unique_ptr<Skybox> skybox_;
};

