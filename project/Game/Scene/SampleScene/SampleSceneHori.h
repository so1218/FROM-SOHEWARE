#pragma once
#include "Engine.h"
#include "Camera.h"
#include "MaterialManager.h"
#include "Model.h"
#include "Sprite.h"
#include "BaseScene.h"
#include "GameObjectManager.h"
#include "Player.h"
#include "Enemy.h"
#include "ShakeEffect.h"
#include "AnimationData.h"
#include "AnimationModel.h"

struct Particle
{
    Vector3 position;
    Vector3 velocity;
    float lifetime;
    bool active;
    ParticleType type;
    uint32_t textureIndex;
};

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

    void ApplyGlobalVariables();
    void SaveGlobalVariables();
private:
    // メンバー変数
    Engine* engine_;
    Camera* camera_;
    GameObjectManager objectManager_;

    std::unique_ptr<Model> dragonModel_;
    std::unique_ptr<Sprite> uvCheckerSprite_;
    std::unique_ptr<Player> player_;
    std::unique_ptr<Enemy> enemy_;


	ModelData modelData_;
    Animation animation_;
    AnimatedModelData animeModelData_;

    Skeleton skeleton_;
    SkinCluster skinCluster_;
    float animationTime_;

    std::unique_ptr<AnimationModel> animationRyu_;

    ShakeEffect shake;

    Vector3 baseTranslation_;
    Vector3 originalTranslation_;

    std::vector<Particle> particles_;

    bool isEditorMode_ = false;


};

