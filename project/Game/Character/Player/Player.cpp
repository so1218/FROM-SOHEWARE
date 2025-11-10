#include "Player.h"
#include "MapChipField.h"
#include "CollisionConfig.h"
#include "PlayScene.h"
#include "GlobalVariables.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "AnimationHandle.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "MathUtils.h"  
#include "Collision.h"   
#include "TimeManager.h"
#include "WeaponKnife.h"
#include <numbers>
#include <algorithm>

Player::Player(Engine* engine, Camera* camera)
{
	engine_ = engine;
	camera_ = camera;

	// 通常モデルを生成
	modelPlayer_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(ModelID::cube)));

	// アニメーションモデルを生成
	animationPlayer_ = std::make_unique<AnimationModel>(engine_,camera_,*ModelHandle::Get(ModelID::walk),AnimationHandle::Get(AnimationID::walk));
}

void Player::Initialize()
{
	// プレイヤーの基本情報を設定
	size_ = { 1.0f, 1.0f, 1.0f };
	moveDirection_ = { 0.0f, 0.0f, 0.0f };
	moveSpeed_ = 0.2f;

	// 衝突判定の属性設定
	SetCollisionAttribute(kCollisionAttributePlayer);
	SetCollisionMask(kCollisionAttributeEnemy);

	// デバッグ用のグローバル変数登録
	GlobalVariables::GetInstance()->CreateGroup(GetGlobalVariableGroupName());
	GlobalVariables::GetInstance()->LoadFiles();
	GlobalVariables::GetInstance()->AddItem(GetGlobalVariableGroupName(),"modelPlayer_->GetTransform().translation_",modelPlayer_->GetTransform().translation_);
}

// グローバル変数の適用処理
void Player::ApplyGlobalVariables()
{
	modelPlayer_->GetTransform().translation_ =
		GlobalVariables::GetInstance()->GetVector3Value(GetGlobalVariableGroupName(),"modelPlayer_->GetTransform().translation_");
}

// 現在の値をグローバル変数に保存
void Player::SaveGlobalVariables()
{
	GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(),"modelPlayer_->GetTransform().translation_",modelPlayer_->GetTransform().translation_);
}

// 武器を追加する処理
void Player::AddWeapon(WeaponType type)
{
	switch (type)
	{
	case WeaponType::Knife:
		// ナイフ武器を追加
		weapons_.push_back(std::make_unique<WeaponKnife>(engine_, this, camera_));
		break;

	default:
		break;
	}
}

// 更新処理
void Player::Update()
{
	// 移動処理
	Move();

	// モデルの行列更新
	modelPlayer_->GetTransform().UpdateMatrix();
	UpdateAABB();

	// アニメーション更新
	animationPlayer_->Update(1, true);
	animationPlayer_->transform_ = modelPlayer_->GetTransform();

	// 所持武器の更新
	for (auto& weapon : weapons_)
	{
		weapon->Update(TimeManager::GetInstance()->GetDeltaTime());
	}
}

void Player::AddWeaponColliders(CollisionManager* manager)
{
	for (auto& weapon : weapons_)
	{
		weapon->AddCollidersToManager(manager);
	}
}

void Player::Move()
{
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

	moveDirection_ = GetMoveDirection();

	// 移動方向がある場合、最後の移動方向を更新
	if (moveDirection_.Length() > 0.0f)
	{
		lastMoveDirection_ = moveDirection_;
	}

	// 向きを補間して滑らかに回転させる
	if (lastMoveDirection_.Length() > 0.001f)
	{
		float targetAngleY = std::atan2(lastMoveDirection_.x, lastMoveDirection_.z);
		Quaternion targetRotation = Quaternion::QuaternionFromEuler({ 0.0f, targetAngleY, 0.0f });

		Quaternion currentRotation = modelPlayer_->GetTransform().rotationQuaternion_;
		float slerpFactor = Math::Clamp(rotationSpeed_ * deltaTime, 0.0f, 1.0f);
		Quaternion newRotation = Quaternion::Slerp(currentRotation, targetRotation, slerpFactor);

		modelPlayer_->GetTransform().rotationQuaternion_ = newRotation;
	}

	// 実際の位置更新
	modelPlayer_->GetTransform().translation_ += moveDirection_ * moveSpeed_;
	modelPlayer_->GetTransform().translation_.y = 0.5f; // 地面の高さを固定
}

// 入力から移動方向を取得
Vector3 Player::GetMoveDirection()
{
	Vector3 dir = { 0.0f, 0.0f, 0.0f };
	auto& input = Input::GetInstance();

	const int controllerId = 0; // 1P想定
	SHORT stickX = input.GetLeftStickX(controllerId);
	SHORT stickY = input.GetLeftStickY(controllerId);
	const int DEAD_ZONE = STICK_THRESHOLD;

	float normalizedX = 0.0f;
	float normalizedY = 0.0f;

	// スティックの入力を正規化
	if (abs(stickX) > DEAD_ZONE)
		normalizedX = static_cast<float>(stickX) / 32768.0f;
	if (abs(stickY) > DEAD_ZONE)
		normalizedY = static_cast<float>(stickY) / 32768.0f;

	// キーボード入力対応
	if (input.IsKeyPressed(DIK_W)) normalizedY += 1.0f;
	if (input.IsKeyPressed(DIK_S)) normalizedY -= 1.0f;
	if (input.IsKeyPressed(DIK_D)) normalizedX += 1.0f;
	if (input.IsKeyPressed(DIK_A)) normalizedX -= 1.0f;

	dir = { normalizedX, 0.0f, normalizedY };

	// カメラ方向に基づいて移動ベクトルを変換
	if (dir.Length() > 0.0f)
	{
		dir = dir.Normalize();

		Vector3 cameraForward = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f });
		Vector3 cameraRight = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 1.0f, 0.0f, 0.0f });

		cameraForward.y = 0.0f;
		cameraRight.y = 0.0f;
		cameraForward = cameraForward.Normalize();
		cameraRight = cameraRight.Normalize();

		dir = cameraForward * dir.z + cameraRight * dir.x;
		dir = dir.Normalize();
	}

	return dir;
}

void Player::UpdateAABB()
{
	Vector3 center = modelPlayer_->GetTransform().GetWorldPosition();
	float halfW = size_.x / 2.0f;
	float halfH = size_.y / 2.0f;
	float halfD = size_.z / 2.0f;

	aabb_.min = { center.x - halfW, center.y - halfH, center.z - halfD };
	aabb_.max = { center.x + halfW, center.y + halfH, center.z + halfD };
}

void Player::OnCollision(Collider* other)
{
}

Vector3 Player::GetWorldPosition()
{
	Vector3 worldPos;
	worldPos.x = modelPlayer_->GetTransform().matWorld_.m[3][0];
	worldPos.y = modelPlayer_->GetTransform().matWorld_.m[3][1];
	worldPos.z = modelPlayer_->GetTransform().matWorld_.m[3][2];
	return worldPos;
}

void Player::Draw()
{
	modelPlayer_->Draw();
	animationPlayer_->Draw();

	for (auto& weapon : weapons_)
	{
		weapon->Draw();
	}
}

void Player::DebugDraw()
{
	ImGui::Begin("プレイヤー");
	ImGui::DragFloat3("Transform Translation", &modelPlayer_->GetTransform().translation_.x, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("Transform Scale", &modelPlayer_->GetTransform().scale_.x, 0.1f, -100.0f, 100.0f);
	ImGui::End();

	for (auto& weapon : weapons_)
	{
		weapon->DebugDraw();
	}
}