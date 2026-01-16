#pragma once
#include "BaseScene.h"
#include "Player.h"
#include "Enemy.h"
#include "ShakeEffect.h"
#include "FollowCamera.h"
#include "EnemyManager.h"
#include "ParticleEmitter.h"
#include "Skybox.h"
#include "GameTimer.h"
#include "LevelUpManager.h" 
#include "LevelUpUI.h"

class SampleSceneHori : public BaseScene
{
public:
	SampleSceneHori(Engine* engine, Camera* camera);

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

    // 通常プレイ中の更新処理
    void UpdatePlaying();
    // レベルアップ選択画面中の更新処理
    void UpdateLevelUpSelection();

    enum class SceneState
    {
        Playing,    
        LevelUpSelection 
    };

private:
    // メンバー変数
    std::unique_ptr<EnemyManager> enemyManager_;
    std::unique_ptr<ParticleEmitter> playerWalkEmitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> playerLevelUpEmitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> playerDamagedEmitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> playerGetExpEmitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> sceneEmitter_ = nullptr;

    Player* player_ = nullptr;
    Enemy* enemy_ = nullptr;
    FollowCamera* followCamera_ = nullptr;
    GameTimer* gameTimer_ = nullptr;

    std::unique_ptr<Skybox> skybox_;

    SceneState sceneState_ = SceneState::Playing;

    std::unique_ptr<LevelUpManager> levelUpManager_;
    std::unique_ptr<LevelUpUI> levelUpUI_;
};

