#pragma once

#include "BaseCharacter.h"
#include "Collider.h"
#include "ModelHandle.h"
#include "AnimationModel.h"
#include "AnimationHandle.h"
#include "GameObjectManager.h"

class Player;

struct EnemyData
{
    ModelID modelId = ModelID::walk; // 使用するモデル
    AnimationID animationId = AnimationID::walk;
    float hp = 50.0f;
    float speed = 1.4f;
    Vector3 size = { 1.0f, 1.0f, 1.0f };
};

class Enemy : public Collider, public BaseCharacter
{
public:
    Enemy(Engine* engine, Camera* camera, Player* player, GameObjectManager* objectManager, const EnemyData& data);

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
    void ApplyGlobalVariables();
    std::vector<std::string> GetGlobalVariableGroupName() const { return { "Enemy" }; }

    // 衝突・ダメージ処理
    void OnCollision(Collider* other) override;
    void TakeDamage(float damage, const Vector3& hitSourcePosition); // ダメージを受ける関数を追加
    void SpawnExperienceGem();  // 経験値を生成する関数
    bool IsDead() const override { return isDead_; } // 死亡フラグ

    // 座標・当たり判定
    Vector3 GetWorldPosition() override;
    void UpdateAABB();
    WorldTransform& GetWorldTransform() { return modelEnemy_->GetTransform(); }
    AABB& GetAABB() { return aabb_; }

private:
    Engine* engine_;
    Camera* camera_;
    Player* player_;
    GameObjectManager* objectManager_;

    std::unique_ptr<Model> modelEnemy_;
    std::unique_ptr<AnimationModel> animationEnemy_;

    AABB aabb_;

    // 敵のステータス
    Vector3 size_;
    float hp_;     // 現在のHP
    float speed_;  // 移動速度
    bool isDead_ = false; // 死亡フラグ

    // 白く光らせるためのタイマー
    int flashTimer_ = 0;
    static const int kFlashDuration_ = 15; // 5フレーム光る

    // ノックバック関連
    Vector3 knockbackVelocity_ = { 0.0f, 0.0f, 0.0f }; // ノックバック速度
    float knockbackFriction_ = 0.8f; // ノックバックの減衰率 (小さいほどすぐ止まる)
    float knockbackPower_ = 1.0f;    // ノックバックの強さ
};