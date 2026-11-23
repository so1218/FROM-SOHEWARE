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
class PlayerMoneyDisplay;
class PlayerConcealment;
class PlayerBurn;
class GrassField;

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

		float aimMoveSpeed = 0.08f;       
		float aimIdleAnimSpeed = 0.5f;
		float aimWalkAnimSpeed = 0.8f;
		float aimBlendTime = 0.1f;        

		// 武器威力・弾道調整
		float maxDamageMultiplier = 1.5f;    
		float maxBulletSpread = 0.04f;       

		// ダッシュ
		float sprintSpeedRate = 1.5f;         
		float sprintAnimSpeed = 1.3f;
		float sprintStaminaPerSecond = 20.0f;  

		// 回避
		float dodgeDistance = 12.0f;
		float dodgeDuration = 0.4f;
		float dodgeStaminaCost = 30.0f;
		float dodgeAnimSpeed = 1.6f;

		// 殴られたとき
		float hitKnockbackDistance = 3.0f;  
		float hitKnockbackTime = 0.25f;     
		float hitStunTime = 0.4f;           
		float hitInvulnerableTime = 0.8f;   
		float hitCameraShake = 0.04f;       
		float hitStopTime = 0.05f;          
		float hitStopTimeScale = 0.05f;     

		float jumpStaminaCost = 15.0f;

		// 這う
		float crawlSpeed = 0.1f;
		float crawlWalkAnimSpeed = 0.7f;
		float crawlIdleAnimSpeed = 1.0f;
		float crawlBlendTime = 0.2f;
		float crawlColliderHeightRate = 0.6f;
	};

	Config config;

public:
	Player(FS::Engine* engine, FS::Camera* camera, FS::Terrain* terrain, TreeField* treeField);
	~Player() override;

	void Initialize() override;
	void Update() override;
	void Draw() override;
	void DebugGui() override;
	void OnCollisionEnter(FS::Collider* mine, FS::Collider* other) override;

	void FireWeapon();
	void TakeDamage(int damage); 
	void TakeHit(int damage, const FS::Vector3& knockbackDirection);
	// 燃えている草の上で受ける継続ダメージ
	void TakeBurnDamage(int damage);

	void ResetToSpawn();
	// HP が無くなって倒れている
	bool IsDead() const;
	int GetShotCount() const { return shotCount_; } 

	// 所持金
	void AddMoney(int amount);
	bool TrySpend(int amount);  
	int GetMoney() const { return money_; }
	void UpdateAimRotation(); 

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

	// カメラから見た移動の入力
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

	// FollowCamera はプレイヤーを追いかけるので、プレイヤーを作ったあとで渡す
	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }
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
	FS::Vector3 GetColliderCenter() { return GetTransform().translation_ + GetCurrentColliderOffset(); }
	FS::Vector3 GetColliderHalfSize() const { return GetCurrentColliderSize() * 0.5f; }
	// 這っている間は低くなる
	FS::Vector3 GetCurrentColliderSize() const;
	FS::Vector3 GetCurrentColliderOffset() const;
	bool IsSprintInputActive() const { return isSprintInputActive_; }
	// ダッシュの入力を取り消す。押したままのボタンは、押し直すまでダッシュとして扱わない
	void CancelSprintInput();
	bool IsDodgeInputTriggered() const;
	bool IsAimInputActive() const;
	bool IsJumpInputTriggered() const;
	bool IsShootInputTriggered() const;
	bool IsReloadInputTriggered() const;
	bool IsCrawlInputTriggered() const;

	// 這うと隠れる
	bool IsCrawling() const;
	// 敵から見つかりにくい
	bool IsHidden() const;
	void SetGrassField(const GrassField* grassField);
	PlayerWeapon* GetWeapon() const { return weapon_.get(); }

	// false の間は入力を受け付けず、HUD も出さない
	void SetControllable(bool isControllable) { isControllable_ = isControllable; }
	bool IsControllable() const { return isControllable_; }

	// true の間は敵に気づかれず、狙われていても見失われる
	void SetIgnoredByEnemies(bool isIgnored) { isIgnoredByEnemies_ = isIgnored; }
	bool IsIgnoredByEnemies() const { return isIgnoredByEnemies_; }
	// 敵が気づける状態か
	bool CanBeNoticed() const { return isControllable_ && !isIgnoredByEnemies_; }

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
	std::unique_ptr<PlayerMoneyDisplay> moneyDisplay_;
	int money_ = 0;
	std::unique_ptr<PlayerReticle> reticle_;

	FS::Collider* collider_ = nullptr; 
	std::unique_ptr<FS::PropertyBinder> binder_;

	FS::Vector3 moveDirection_{ 0.0f, 0.0f, 0.0f };
	FS::Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 1.0f };
	float velocityY_ = 0.0f; // Y軸の速度

	std::unique_ptr<StateMachine<Player>> stateMachine_;

	// コライダー調整用の変数
	FS::Vector3 colliderOffset_ = { 0.0f, 1.0f, 0.0f };
	FS::Vector3 colliderSize_ = { 1.0f, 2.0f, 1.0f };

	// ワールドインタラクション調整用パラメータ
	FS::Vector3 prevPosition_{ 0.0f, 0.0f, 0.0f };
	float interactionRadius_ = 1.5f;
	float interactionForce_ = 1.0f;
	float maxVerticalDist_ = 2.0f;

	bool isGroundedOnObject_ = false; 

	float prevAnimNormalizedTime_ = 0.0f;

	// ダッシュ・回避
	void UpdateSprintInput();

	std::unique_ptr<PlayerStamina> stamina_;
	std::unique_ptr<PlayerHealth> health_;
	std::unique_ptr<PlayerConcealment> concealment_;
	std::unique_ptr<PlayerBurn> burn_;
	void PlaceAtSpawn();
	// 倒れる
	void StartDeath();
	// 当たった瞬間に一瞬だけ時間を遅くして、衝撃を強調する
	void StartHitStop();
	void UpdateHitStop();
	void EndHitStop();
	FS::Vector3 spawnPosition_ = { 0.0f, 0.0f, 0.0f };  
	float spawnRotationY_ = 0.0f;                        
	bool isSprintInputActive_ = false;
	bool isControllerSprintLatched_ = false;   
	bool isSprintRepressRequired_ = false;

	int shotCount_ = 0;

	// 殴られたとき
	float invulnerableTimer_ = 0.0f;
	bool isHitStopActive_ = false;
	float hitStopTimer_ = 0.0f;
	float timeScaleBeforeHitStop_ = 1.0f;
	bool isControllable_ = true;
	bool isIgnoredByEnemies_ = false;
};

