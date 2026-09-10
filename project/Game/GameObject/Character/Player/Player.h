#pragma once
#include "Collider.h"
#include "AnimationModel.h"
#include "FollowCamera.h"
#include "PropertyBinder.h"
#include "GameObject.h"
#include "StateMachine.h"
#include "ParticleEmitter.h"
#include "Terrain.h"
#include "TreeField.h"

class PlayerStateNormal; 

class Player : public FE::GameObject
{
public:
	// 調整用パラメータ構造体
	struct Config
	{
		float runSpeed = 0.2f;
		float rotationSpeed = 10.0f;

		// アニメーション速度・ブレンド時間
		float idleAnimSpeed = 1.0f;
		float runAnimSpeed = 1.0f;
		float jumpAnimSpeed = 1.0f;         
		float idleToRunBlendTime = 0.15f;
		float runToIdleBlendTime = 0.20f;
		float jumpBlendTime = 0.10f;       

		// ジャンプ・物理用パラメータ
		float jumpInitialVelocity = 10.0f;
		float gravity = 29.8f;
		float airControlRate = 0.8f;     
	};

	Config config;

public:
	Player(FE::Engine* engine, FE::Camera* camera);

	void Initialize() override;
	void Update() override;
	void Draw() override;
	void DebugDraw() override;
	void OnCollisionEnter(FE::Collider* mine, FE::Collider* other) override;

	// Stateから呼び出すヘルパー関数
	void UpdateRotation(const FE::Vector3& moveDir);
	void ApplyHorizontalMovement(const FE::Vector3& moveDir, float speed);
	void ApplyGravity(float deltaTime);
	void SnapToGround();
	bool IsGrounded() const;

	// 共通アクセサ
	void PlayAnimation(const std::string& name, bool loop = true, float speed = 1.0f, float blendTime = 0.2f) {
		animationModel_->Play(name, loop, speed, blendTime);
	}

	void SetMoveDirection(const FE::Vector3& dir) { moveDirection_ = dir; }
	FE::Vector3 GetMoveDirection();
	FE::Vector3 GetLastMoveDirection() const { return lastMoveDirection_; }

	void SetVelocityY(float vy) { velocityY_ = vy; }
	float GetVelocityY() const { return velocityY_; }

	FE::Camera* GetCamera() const { return camera_; }
	StateMachine<Player>* GetStateMachine() { return stateMachine_.get(); }

	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }
	void SetTerrain(FE::Terrain* terrain) { terrain_ = terrain; }
	void SetTreeField(TreeField* treeField) { treeField_ = treeField; }
	// オブジェクト上の接地フラグ設定
	void SetGroundedOnObject(bool grounded) { isGroundedOnObject_ = grounded; }

	std::unique_ptr<FE::AnimationModel> animationModel_;

private:
	FE::Engine* engine_;
	FE::Camera* camera_ = nullptr;
	FE::Terrain* terrain_ = nullptr;
	TreeField* treeField_ = nullptr;
	FollowCamera* followCamera_ = nullptr;

	std::unique_ptr<FE::Model> weaponModel_;
	FE::WorldTransform rightHandTransform_;
	FE::Vector3 weaponOffsetPos_ = { 0.0f, 0.1f, 0.0f };

	std::unique_ptr<FE::Collider> collider_;
	std::unique_ptr<FE::PropertyBinder> binder_;
	std::unique_ptr<FE::ParticleEmitter> auraEmitter_ = nullptr;

	FE::Vector3 moveDirection_{ 0.0f, 0.0f, 0.0f };
	FE::Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 1.0f };
	float velocityY_ = 0.0f; // Y軸の速度

	std::unique_ptr<StateMachine<Player>> stateMachine_;

	// コライダー調整用の変数
	FE::Vector3 colliderOffset_ = { 0.0f, 1.0f, 0.0f };
	FE::Vector3 colliderSize_ = { 0.5f, 1.0f, 0.5f };

	// ワールドインタラクション調整用パラメータ
	FE::Vector3 prevPosition_{ 0.0f, 0.0f, 0.0f };
	float interactionRadius_ = 1.5f;
	float interactionForce_ = 1.0f;
	float maxVerticalDist_ = 2.0f;

	bool isGroundedOnObject_ = false; // オブジェクトの上に乗っているか
};

