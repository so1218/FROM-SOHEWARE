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

class PlayerWeapon;
class PlayerReticle;
class PlayerStamina;
class PlayerHealth;
class PlayerAmmoDisplay;

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

		// ダッシュ
		float sprintSpeedRate = 1.5f;          // 走りの速さに対する倍率
		float sprintAnimSpeed = 1.3f;
		float sprintStaminaPerSecond = 20.0f;  // 1 秒あたりに使うスタミナ

		// 回避
		float dodgeDistance = 12.0f;
		float dodgeDuration = 0.4f;
		float dodgeStaminaCost = 30.0f;
		float dodgeAnimSpeed = 1.6f;

		float jumpStaminaCost = 15.0f;
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
	void TakeDamage(int damage); // 回避中は当たらない
	// 初期位置に戻して HP やスタミナも元に戻す
	void ResetToSpawn();
	int GetShotCount() const { return shotCount_; } // これまでに撃った数 (チュートリアルの判定用)
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

	// カメラから見た移動の入力。フレームの初めに1回だけ読む
	const FS::Vector3& GetMoveDirection() const { return moveDirection_; }
	bool IsMoving() const { return moveDirection_.LengthSq() > kMovingThreshold * kMovingThreshold; }
	FS::Vector3 GetLastMoveDirection() const { return lastMoveDirection_; }

	// 入力があり、スタミナが足りれば、その動きを始める
	bool TryStartDodge();
	bool TryStartJump();

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

	// ダッシュ・回避
	PlayerStamina* GetStamina() const { return stamina_.get(); }
	PlayerHealth* GetHealth() const { return health_.get(); }
	FS::Vector3 GetColliderCenter() { return GetTransform().translation_ + colliderOffset_; }
	const FS::Vector3& GetColliderSize() const { return colliderSize_; }
	bool IsSprintInputActive() const { return isSprintInputActive_; }
	bool IsDodgeInputTriggered() const;
	bool IsAimInputActive() const;
	bool IsJumpInputTriggered() const;
	bool IsShootInputTriggered() const;
	bool IsReloadInputTriggered() const;
	PlayerWeapon* GetWeapon() const { return weapon_.get(); }

	// false の間は入力を受け付けず、HUD も出さない (タイトル画面など)
	void SetControllable(bool isControllable) { isControllable_ = isControllable; }
	bool IsControllable() const { return isControllable_; }

private:
	static constexpr float kMovingThreshold = 0.1f;

	FS::Vector3 ReadMoveInput() const;

	FS::Engine* engine_;
	FS::Camera* camera_ = nullptr;
	FS::Terrain* terrain_ = nullptr;
	TreeField* treeField_ = nullptr;
	FollowCamera* followCamera_ = nullptr;

	std::unique_ptr<PlayerWeapon> weapon_;
	std::unique_ptr<PlayerAmmoDisplay> ammoDisplay_;
	std::unique_ptr<PlayerReticle> reticle_;

	FS::Collider* collider_ = nullptr; // 所有は GameObject (AddCollider で作成)
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

	// ダッシュ・回避
	void UpdateSprintInput();

	std::unique_ptr<PlayerStamina> stamina_;
	std::unique_ptr<PlayerHealth> health_;
	void PlaceAtSpawn();
	FS::Vector3 spawnPosition_ = { 0.0f, 0.0f, 0.0f };   // 始まりの位置。倒れたときもここに戻る
	float spawnRotationY_ = 0.0f;                         // 始まりの向き (度)
	bool isSprintInputActive_ = false;
	bool isControllerSprintLatched_ = false;   // コントローラーは L3 でダッシュを始め、スティックを離すまで続ける

	int shotCount_ = 0;
	bool isControllable_ = true;
};

