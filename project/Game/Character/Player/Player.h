#pragma once

#include "BaseCharacter.h"
#include "Collider.h"

class PlayScene;

class Player : public Collider, public BaseCharacter
{
public:
	Player();

	GameObjectType GetType() const override { return GameObjectType::Player; }

	// 初期化
	void Initialize(Engine* engine, Camera* camera) override;

	// 更新
	void Update() override;

	// 描画
	void Draw() override;

	// デバッグ描画処理
	void DebugDraw() override;

	// 衝突を検出したら呼び出されるコールバック関数
	void OnCollision() override;

	// 調整項目の適用
	void ApplyGlobalVariables() override;
	const char* GetGlobalVariableGroupName() const override { return "Player"; }

	// AABBを取得
	void UpdateAABB();
	// ワールド座標を取得
	Vector3 GetWorldPosition() override;
	
	// ゲッター
	WorldTransform& GetWorldTransform() { return modelPlayer_->GetTransform(); }
	AABB& GetAABB() { return aabb_;	}

private:
	
	Engine* engine_;
	Camera* camera_;

	std::unique_ptr<Model> modelPlayer_;
	AABB aabb_;
	
	// キャラクターの当たり判定サイズ
	Vector3 size_;
	
};

