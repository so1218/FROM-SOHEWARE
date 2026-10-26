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
class PlayerWeapon;
class PlayerReticle;

class Player : public FS::GameObject
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

		float aimMoveSpeed = 0.08f;       // エイム中の移動速度
		float aimToIdleBlendTime = 0.1f;
		float shootRecoilTime = 0.15f;    // 射撃後の隙・反動時間

		// 武器威力・弾道調整
		float maxDamageMultiplier = 1.5f;    // 完全フォーカス時の威力倍率
		float maxBulletSpread = 0.04f;       // 未フォーカス時の弾道の最大ブレ角
	};

	Config config;

public:
	Player(FS::Engine* engine, FS::Camera* camera);

	void Initialize() override;
	void Update() override;
	void Draw() override;
	void DebugDraw() override;
	void OnCollisionEnter(FS::Collider* mine, FS::Collider* other) override;

	void FireWeapon(); // 射撃時の処理
	void UpdateAimRotation(); // エイム時カメラの正面に体を向ける

	// Stateから呼び出すヘルパー関数
	void UpdateRotation(const FS::Vector3& moveDir);
	void ApplyHorizontalMovement(const FS::Vector3& moveDir, float speed);
	void ApplyGravity();
	void SnapToGround();
	bool IsGrounded() const;

	// 共通アクセサ
	void PlayAnimation(const std::string& name, bool loop = true, float speed = 1.0f, float blendTime = 0.2f) {
		animationModel_->Play(name, loop, speed, blendTime);
	}

	void SetMoveDirection(const FS::Vector3& dir) { moveDirection_ = dir; }
	FS::Vector3 GetMoveDirection();
	FS::Vector3 GetLastMoveDirection() const { return lastMoveDirection_; }

	void SetVelocityY(float vy) { velocityY_ = vy; }
	float GetVelocityY() const { return velocityY_; }

	FS::Camera* GetCamera() const { return camera_; }
	PlayerReticle* GetReticle() const { return reticle_.get(); }
	FollowCamera* GetFollowCamera() const { return followCamera_; }
	StateMachine<Player>* GetStateMachine() { return stateMachine_.get(); }

	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }
	void SetTerrain(FS::Terrain* terrain) { terrain_ = terrain; }
	void SetTreeField(TreeField* treeField) { treeField_ = treeField; }
	// オブジェクト上の接地フラグ設定
	void SetGroundedOnObject(bool grounded) { isGroundedOnObject_ = grounded; }

	std::unique_ptr<FS::AnimationModel> animationModel_;

	// 移動中に毎フレーム呼び出す足音更新関数
	void UpdateFootstepEvents();
	// 移動停止時に足音タイマー等をリセットする関数
	void ResetFootstepState();

private:
	FS::Engine* engine_;
	FS::Camera* camera_ = nullptr;
	FS::Terrain* terrain_ = nullptr;
	TreeField* treeField_ = nullptr;
	FollowCamera* followCamera_ = nullptr;

	std::unique_ptr<PlayerWeapon> weapon_;
	std::unique_ptr<PlayerReticle> reticle_;

	std::unique_ptr<FS::Collider> collider_;
	std::unique_ptr<FS::PropertyBinder> binder_;
	std::unique_ptr<FS::ParticleEmitter> auraEmitter_ = nullptr;

	FS::Vector3 moveDirection_{ 0.0f, 0.0f, 0.0f };
	FS::Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 1.0f };
	float velocityY_ = 0.0f; // Y軸の速度

	std::unique_ptr<StateMachine<Player>> stateMachine_;

	// コライダー調整用の変数
	FS::Vector3 colliderOffset_ = { 0.0f, 1.0f, 0.0f };
	FS::Vector3 colliderSize_ = { 0.5f, 1.0f, 0.5f };

	// ワールドインタラクション調整用パラメータ
	FS::Vector3 prevPosition_{ 0.0f, 0.0f, 0.0f };
	float interactionRadius_ = 1.5f;
	float interactionForce_ = 1.0f;
	float maxVerticalDist_ = 2.0f;

	bool isGroundedOnObject_ = false; // オブジェクトの上に乗っているか

	float prevAnimNormalizedTime_ = 0.0f; // 前フレームのアニメーション進捗率
};

