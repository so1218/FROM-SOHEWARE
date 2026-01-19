#pragma once

#include "BaseCharacter.h"
#include "Collider.h"
#include "Weapon.h"
#include "AnimationModel.h"
#include "FollowCamera.h"
#include "UpgradeInfo.h"
#include "Line.h"

class PlayScene;

enum class PlayerAnimState
{
	None, 
	Idle, 
	Walk  
};

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

	// 調整項目の適用
	void ApplyGlobalVariables();
	std::vector<std::string> GetGlobalVariableGroupName() const { return { "Player" }; }

	void AddWeapon(WeaponType type); 
	void AddWeaponColliders(CollisionManager* manager);

	// 移動処理
	void Move();

	// ワールド座標を取得
	Vector3 GetWorldPosition() const override;
	
	// ゲッター
	WorldTransform& GetWorldTransform() { return modelPlayer_->GetTransform(); }
	AABB& GetAABB() { return aabb_;	}

	Vector3 GetMoveDirection();

	// 武器が「照準」に使うための公開関数
	Vector3 GetLastMoveDirection() const { return lastMoveDirection_; }
	// カメラを返す
	Camera* GetCamera() const { return camera_; }

	// 経験値取得、ダメージ
	void GainExperience(int amount);
	void TakeDamage(float damage);

	std::unique_ptr<Model> modelPlayer_;
	std::unique_ptr<Model> modelTamesi_;

	void SetWalkEmitter(ParticleEmitter* emitter) { walkEmitterPtr_ = emitter; }
	ParticleEmitter* walkEmitterPtr_ = nullptr;
	void SetLevelUpEmitter(ParticleEmitter* emitter) { levelUpEmitterPtr_ = emitter; }
	ParticleEmitter* levelUpEmitterPtr_ = nullptr;
	void SetDamagedEmitter(ParticleEmitter* emitter) { damagedEmitterPtr_ = emitter; }
	ParticleEmitter* damagedEmitterPtr_ = nullptr;
	void SetGetExpEmitter(ParticleEmitter* emitter) { getExpEmitterPtr_ = emitter; }
	ParticleEmitter* getExpEmitterPtr_ = nullptr;

	// UIが必要とする情報
	float GetHpRatio() const;
	float GetXpRatio() const;
	int GetLevel() const { return level_; }

	bool IsEnd() const { return isEnd_; }

	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }
	FollowCamera* followCamera_;

	// 選択された強化を適用する関数
	void ApplyUpgrade(const UpgradeInfo& upgrade);

	// 外部から状態を取得、変更するための関数
	bool IsWaitingForUpgrade() const { return isWaitingForUpgrade_; }
	void FinishUpgrade() { isWaitingForUpgrade_ = false; }

	bool HasWeapon(WeaponType type) const; 
	// 武器リストへのアクセサ
	const std::vector<std::unique_ptr<Weapon>>& GetWeapons() const { return weapons_; }

private:

	// レベルアップの内部処理
	void LevelUp();

	Camera* camera_ = nullptr;

	std::unique_ptr<AnimationModel> animationPlayer_;
	AABB aabb_;
	
	// キャラクターの当たり判定サイズ
	Vector3 size_;
	
	Vector3 moveDirection_;
	float moveSpeed_;

	float rotationSpeed_ = 10.0f;

	// 武器の設計図のリスト
	std::vector<std::unique_ptr<Weapon>> weapons_;
	Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 1.0f };

	// HP/ダメージ関連の変数
	float maxHp_ = 100.0f;
	float hp_ = 100.0f;
	bool isInvincible_ = false;      // 無敵中か
	float invincibilityTimer_ = 0.0f; // 無敵時間タイマー
	float invincibilityDuration_ = 1.0f; // 無敵時間の長さ

	// 経験値/レベル関連の変数
	int experience_ = 0;      // 現在の経験値
	int xpToNextLevel_ = 10;  // 次のレベルアップに必要な経験値
	int level_ = 1;
	bool isEnd_ = false; // 死亡フラグ

	bool isWaitingForUpgrade_ = false; // 選択待ちフラグ

	PlayerAnimState currentAnimState_ = PlayerAnimState::None;
};

