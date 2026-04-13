#pragma once
#include "Collider.h"
#include "AnimationModel.h"
#include "FollowCamera.h"
#include "Line.h"
#include "PropertyBinder.h"

class PlayScene;

class Player : public FE::Collider, public FE::GameObject
{
public:
	Player(FE::Engine* engine, FE::Camera* camera);

	// 初期化
	void Initialize() override;

	// 更新
	void Update() override;

	// 描画
	void Draw() override;

	// デバッグ描画処理
	void DebugDraw() override;

	// 衝突を検出したら呼び出されるコールバック関数
	void OnCollisionEnter(FE::Collider* other) override;

	// 移動処理
	void Move();

	// ワールド座標を取得
	FE::Vector3 GetWorldPosition() const override;

	FE::Vector3 GetMoveDirection();

	FE::Vector3 GetLastMoveDirection() const { return lastMoveDirection_; }

	FE::Camera* GetCamera() const { return camera_; }

	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }

	std::unique_ptr<FE::AnimationModel> animationPlayer_;

private:
	FE::Engine* engine_;
	FE::Camera* camera_ = nullptr;
	FollowCamera* followCamera_;

	std::unique_ptr<FE::PropertyBinder> binder_;
	
	FE::Vector3 moveDirection_;
	float moveSpeed_;

	float rotationSpeed_ = 10.0f;

	FE::Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 1.0f };

	// HP/ダメージ関連の変数
	float maxHp_ = 100.0f;
	float hp_ = 100.0f;

};

