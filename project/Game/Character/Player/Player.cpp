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

	modelPlayer_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(ModelID::cube)));
	animationPlayer_ = std::make_unique<AnimationModel>(
		engine_,
		camera_,
		*ModelHandle::Get(ModelID::walk),
		AnimationHandle::Get(AnimationID::walk)
	);
}

void Player::Initialize()
{
	size_ = { 1.0f, 1.0f, 1.0f };
	moveDirection_ = { 0.0f, 0.0f, 0.0f };
	moveSpeed_ = 0.2f;

	// 衝突属性を設定
	SetCollisionAttribute(kCollisionAttributePlayer);
	// 衝突対象を自分の属性以外に設定
	SetCollisionMask(kCollisionAttributeEnemy);

	// グループ名を追加
	GlobalVariables::GetInstance()->CreateGroup(GetGlobalVariableGroupName());
	GlobalVariables::GetInstance()->LoadFiles();
	GlobalVariables::GetInstance()->AddItem(GetGlobalVariableGroupName(),"modelPlayer_->GetTransform().translation_", modelPlayer_->GetTransform().translation_);
}

void Player::ApplyGlobalVariables()
{
	modelPlayer_->GetTransform().translation_ = GlobalVariables::GetInstance()->GetVector3Value(
		GetGlobalVariableGroupName(), "modelPlayer_->GetTransform().translation_");
}


void Player::SaveGlobalVariables()
{
	GlobalVariables::GetInstance()->SetValue(GetGlobalVariableGroupName(), "modelPlayer_->GetTransform().translation_", modelPlayer_->GetTransform().translation_);
}

void Player::AddWeapon(WeaponType type)
{
	// 注文書(type)を見て、正しい武器を "製造" する
	switch (type)
	{
	case WeaponType::Knife:
		weapons_.push_back(std::make_unique<WeaponKnife>(engine_, this));
		break;

	default:
		// 該当なし（エラー）
		assert(false && "未定義の武器タイプが指定されました");
		break;
	}
}

void Player::Update()
{
	Move();

	animationPlayer_->Update(1, true);
	animationPlayer_->transform_ = modelPlayer_->GetTransform();
	for (auto& weapon : weapons_)
	{
		weapon->Update(TimeManager::GetInstance()->GetDeltaTime());
	}
}

void Player::Move()
{
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

	moveDirection_ = GetMoveDirection();

	if (moveDirection_.Length() > 0.0f)
	{
		lastMoveDirection_ = moveDirection_;
	}

	if (lastMoveDirection_.Length() > 0.001f)
	{
		// 1. 目標の「角度」を計算 
		float targetAngleY = std::atan2(lastMoveDirection_.x, lastMoveDirection_.z);

		// 2. 目標の角度から「目標のクォータニオン」を計算
		Quaternion targetRotation = Quaternion::QuaternionFromEuler({ 0.0f, targetAngleY, 0.0f });

		// 3. 現在の「クォータニオン」を取得
		Quaternion currentRotation = modelPlayer_->GetTransform().rotationQuaternion_;

		// 4. Slerp (球面線形補間) を実行
		float slerpFactor = Math::Clamp(rotationSpeed_ * deltaTime, 0.0f, 1.0f);

		Quaternion newRotation = Quaternion::Slerp(currentRotation, targetRotation, slerpFactor);

		// 5. 補間された「クォータニオン」をモデルにセット
		modelPlayer_->GetTransform().rotationQuaternion_ = newRotation;

	}

	modelPlayer_->GetTransform().translation_ += moveDirection_ * moveSpeed_;
	modelPlayer_->GetTransform().translation_.y = 0.5f;
}

Vector3 Player::GetMoveDirection()
{
	Vector3 dir = { 0.0f, 0.0f, 0.0f };

	if (Input::GetInstance().IsKeyPressed(DIK_W)) dir.z += 1.0f;
	if (Input::GetInstance().IsKeyPressed(DIK_S)) dir.z -= 1.0f;
	if (Input::GetInstance().IsKeyPressed(DIK_D)) dir.x += 1.0f;
	if (Input::GetInstance().IsKeyPressed(DIK_A)) dir.x -= 1.0f;

	if (dir.Length() > 0.0f)
	{
		dir = dir.Normalize();

		// カメラの向きに合わせて方向を変換
		Vector3 cameraForward = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 0.0f, 0.0f, 1.0f });
		Vector3 cameraRight = camera_->GetWorldTransform().rotationQuaternion_.RotateVector({ 1.0f, 0.0f, 0.0f });

		// y軸方向は固定
		cameraForward.y = 0.0f;
		cameraRight.y = 0.0f;
		cameraForward = cameraForward.Normalize();
		cameraRight = cameraRight.Normalize();

		dir = cameraForward * dir.z + cameraRight * dir.x;
		dir = dir.Normalize(); // 斜め移動も正規化
	}

	return dir;
}


void Player::UpdateAABB()
{
	Vector3 center = modelPlayer_->GetTransform().GetWorldPosition(); // プレイヤーの基準位置
	float halfW = size_.x / 2.0f;
	float halfH = size_.y / 2.0f;
	float halfD = size_.z / 2.0f;

	aabb_.min = { center.x - halfW, center.y - halfH, center.z - halfD };
	aabb_.max = { center.x + halfW, center.y + halfH, center.z + halfD };
}

void Player::OnCollision()
{

}

Vector3 Player::GetWorldPosition()
{
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得
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

// デバッグ描画処理
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


