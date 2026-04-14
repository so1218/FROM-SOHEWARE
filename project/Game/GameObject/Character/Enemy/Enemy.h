#pragma once
#include "Collider.h"
#include "AnimationModel.h"
#include "GameObjectManager.h"

class Player;

class Enemy : public FE::GameObject
{
public:
    Enemy(FE::Engine* engine);
    ~Enemy();

    // 初期化処理
    void Initialize() override;

    // 更新処理
    void Update() override;

    // 描画処理
    void Draw() override;

    // デバッグ描画処理
    void DebugDraw() override;

    // 衝突を検出したら呼び出されるコールバック関数
    void OnCollisionEnter(FE::Collider* mine, FE::Collider* other) override;

private:
    FE::Engine* engine_;
    std::unique_ptr<FE::AnimationModel> animationEnemy_;
    std::unique_ptr<FE::Collider> collider_;
};