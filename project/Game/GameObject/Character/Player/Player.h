#pragma once
#include "Collider.h"
#include "AnimationModel.h"
#include "FollowCamera.h"
#include "PropertyBinder.h"
#include "GameObject.h"
#include "StateMachine.h"
#include "ParticleEmitter.h"
#include "Terrain.h"

class PlayerStateNormal; 

class Player : public FE::GameObject
{
	friend class PlayerStateNormal;

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
	void OnCollisionEnter(FE::Collider* mine, FE::Collider* other) override;

	// 移動処理
	void Move();

	FE::Vector3 GetMoveDirection();

	FE::Vector3 GetLastMoveDirection() const { return lastMoveDirection_; }

	FE::Camera* GetCamera() const { return camera_; }

	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }

	StateMachine<Player>* GetStateMachine() { return stateMachine_.get(); }

	std::unique_ptr<FE::AnimationModel> animationModel_;

	// 地形情報をセットする関数
	void SetTerrain(FE::Terrain* terrain) { terrain_ = terrain; }

private:
	FE::Engine* engine_;
	FE::Camera* camera_ = nullptr;
	FE::Terrain* terrain_ = nullptr;
	FollowCamera* followCamera_;

	std::unique_ptr<FE::Collider> collider_;

	std::unique_ptr<FE::PropertyBinder> binder_;

	std::unique_ptr<FE::ParticleEmitter> auraEmitter_ = nullptr;
	
	FE::Vector3 moveDirection_;
	float moveSpeed_;

	FE::Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 1.0f };

	std::unique_ptr<StateMachine<Player>> stateMachine_;

	// 調整用パラメータ
	float runSpeed_ = 0.2f;
	float rotationSpeed_ = 10.0f;

	// コライダー調整用の変数
	FE::Vector3 colliderOffset_ = { 0.0f, 1.0f, 0.0f };
	FE::Vector3 colliderSize_ = { 0.5f, 1.0f, 0.5f };
};

