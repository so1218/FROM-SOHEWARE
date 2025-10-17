#pragma once

#include "BaseCharacter.h"
#include "Collider.h"

class Knife;
class PlayScene;

class Player : public Collider, public BaseCharacter
{
public:
	Player(Engine* engine, Camera* camera);

	GameObjectType GetType() const override { return GameObjectType::Player; }

	// 初期化
	void Initialize() override;

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
	void SaveGlobalVariables() override;
	std::vector<std::string> GetGlobalVariableGroupName() const { return { "Player" }; }

	// 移動処理
	void Move();

	// AABBを取得
	void UpdateAABB();
	// ワールド座標を取得
	Vector3 GetWorldPosition() override;
	
	// ゲッター
	WorldTransform& GetWorldTransform() { return modelPlayer_->GetTransform(); }
	AABB& GetAABB() { return aabb_;	}

	Vector3 GetMoveDirection();

private:
	
	Engine* engine_;
	Camera* camera_;

	std::unique_ptr<Model> modelPlayer_;
	std::unique_ptr<Knife> knife_;
	AABB aabb_;
	
	// キャラクターの当たり判定サイズ
	Vector3 size_;
	
	Vector3 moveDirection_;
	float moveSpeed_;
};

