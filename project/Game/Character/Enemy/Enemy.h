#pragma once

#include "BaseCharacter.h"
#include "Collider.h"

class Enemy : public Collider, public BaseCharacter
{
public:
    Enemy(Engine* engine, Camera* camera);

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

    // 衝突を検出したら呼び出されるコールバック関数
    void OnCollision() override;

    // ワールド座標を取得
    Vector3 GetWorldPosition() override;
    // AABBを取得
    void UpdateAABB();

    // ゲッター
    WorldTransform& GetWorldTransform() { return modelEnemy_->GetTransform(); }
    AABB& GetAABB() { return aabb_; }

private:
    Engine* engine_;
    Camera* camera_;

    std::unique_ptr<Model> modelEnemy_;

    AABB aabb_;

    // キャラクターの当たり判定サイズ
    Vector3 size_;
};