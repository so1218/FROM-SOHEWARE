#pragma once

#include "Collider.h"
#include "AnimationModel.h"
#include "GameObjectManager.h"

class Player;

class Enemy : public Collider, public GameObject
{
public:
    Enemy(Engine* engine);
    ~Enemy();

    // 初期化処理
    void Initialize() override;

    // 更新処理
    void Update() override;

    // 描画処理
    void Draw() override;

    // デバッグ描画処理
    void DebugDraw() override;

private:
    std::unique_ptr<AnimationModel> animationEnemy_;
};