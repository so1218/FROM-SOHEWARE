#pragma once

#include "Collider.h"
#include "AnimationModel.h"
#include "FollowCamera.h"
#include "Line.h"
#include "PropertyBinder.h"

class PlayScene;

class Player : public Collider, public GameObject
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
	void OnCollisionEnter(Collider* other) override;

	// 移動処理
	void Move();

	// ワールド座標を取得
	Vector3 GetWorldPosition() const override;

	Vector3 GetMoveDirection();

	Vector3 GetLastMoveDirection() const { return lastMoveDirection_; }
	// カメラを返す
	Camera* GetCamera() const { return camera_; }

	// UIが必要とする情報
	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }
	FollowCamera* followCamera_;

private:

	Camera* camera_ = nullptr;

	std::unique_ptr<AnimationModel> animationPlayer_;
	std::unique_ptr<PropertyBinder> binder_;
	
	Vector3 moveDirection_;
	float moveSpeed_;

	float rotationSpeed_ = 10.0f;

	Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 1.0f };

	// HP/ダメージ関連の変数
	float maxHp_ = 100.0f;
	float hp_ = 100.0f;

};

