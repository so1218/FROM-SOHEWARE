#pragma once

#include "BaseCharacter.h"
#include "Collider.h"
#include "Weapon.h"
#include "AnimationModel.h"

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
	void OnCollision(Collider* other) override;

	// 調整項目の適用
	void ApplyGlobalVariables() override;
	void SaveGlobalVariables() override;
	std::vector<std::string> GetGlobalVariableGroupName() const { return { "Player" }; }

	void AddWeapon(WeaponType type); 

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

	// 武器が「照準」に使うための公開関数
	Vector3 GetLastMoveDirection() const { return lastMoveDirection_; }
	// カメラを返す
	Camera* GetCamera() const { return camera_; }

private:
	
	Engine* engine_;
	Camera* camera_;

	std::unique_ptr<Model> modelPlayer_;
	std::unique_ptr<AnimationModel> animationPlayer_;
	AABB aabb_;
	
	// キャラクターの当たり判定サイズ
	Vector3 size_;
	
	Vector3 moveDirection_;
	float moveSpeed_;

	float rotationSpeed_ = 10.0f;

	// 武器の設計図のリストを持つ。
	std::vector<std::unique_ptr<Weapon>> weapons_;
	Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 1.0f };
};

