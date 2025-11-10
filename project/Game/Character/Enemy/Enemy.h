#pragma once

#include "BaseCharacter.h"
#include "Collider.h"
#include "ModelHandle.h"

class Player;

struct EnemyData
{
    ModelID modelId = ModelID::cube; // 使用するモデル
    float hp = 50.0f;
    float speed = 1.0f;
    Vector3 size = { 1.0f, 1.0f, 1.0f };
};

class Enemy : public Collider, public BaseCharacter
{
public:
    Enemy(Engine* engine, Camera* camera, Player* player, const EnemyData& data);

    GameObjectType GetType() const override { return GameObjectType::Enemy; }

    // 初期化処理
    void Initialize() override;

    // 更新処理
    void Update() override;

    // 描画処理
    void Draw() override;

    // デバッグ描画処理
    void DebugDraw() override;

    // 調整項目の適用
    void ApplyGlobalVariables() override;
    std::vector<std::string> GetGlobalVariableGroupName() const { return { "Enemy" }; }

    // 衝突・ダメージ処理
    void OnCollision(Collider* other) override;
    void TakeDamage(float damage); // ダメージを受ける関数を追加
    bool IsDead() const { return isDead_; } // 死亡フラグ

    // 座標・当たり判定
    Vector3 GetWorldPosition() override;
    void UpdateAABB();
    WorldTransform& GetWorldTransform() { return modelEnemy_->GetTransform(); }
    AABB& GetAABB() { return aabb_; }

private:
    Engine* engine_;
    Camera* camera_;
    Player* player_;

    std::unique_ptr<Model> modelEnemy_;

    AABB aabb_;

    // 敵のステータス
    Vector3 size_;
    float hp_;     // 現在のHP
    float speed_;  // 移動速度
    bool isDead_ = false; // 死亡フラグ
};