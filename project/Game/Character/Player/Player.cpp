#include "Player.h"
#include "MapChipField.h"
#include "CollisionConfig.h"
#include "PlayScene.h"
#include "GlobalVariables.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "MathUtils.h"  
#include "Collision.h"   
#include "TimeManager.h"
#include "Knife.h"

#include <numbers>
#include <algorithm>

Player::Player(Engine* engine, Camera* camera)
{
	engine_ = engine;
	camera_ = camera;

	knife_ = std::make_unique<Knife>(engine_, camera_);
	modelPlayer_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(ModelID::cube)));

	knife_->SetLevel(1);
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


void Player::Update()
{
	Move();
}

void Player::Move()
{
	moveDirection_ = GetMoveDirection();

	modelPlayer_->GetTransform().translation_ += moveDirection_ * moveSpeed_;
	modelPlayer_->GetTransform().translation_.y = 0.5f;
}

Vector3 Player::GetMoveDirection() 
{
	Vector3 dir = { 0.0f, 0.0f, 0.0f };

	if (Input::GetInstance().IsKeyPressed(DIK_W))
	{
		dir.z += 1.0f;
	}
	if (Input::GetInstance().IsKeyPressed(DIK_S))
	{
		dir.z -= 1.0f;
	}
	if (Input::GetInstance().IsKeyPressed(DIK_D))
	{
		dir.x += 1.0f;
	}
	if (Input::GetInstance().IsKeyPressed(DIK_A))
	{
		dir.x -= 1.0f;
	}

	// 正規化（斜め移動で速くなりすぎないように）
	if (dir.Length() > 0.0f)
	{
		dir = dir.Normalize(); 
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
}

// デバッグ描画処理
void Player::DebugDraw()
{
	ImGui::Begin("プレイヤー");
	ImGui::DragFloat3("Transform Translation", &modelPlayer_->GetTransform().translation_.x, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("Transform Scale", &modelPlayer_->GetTransform().scale_.x, 0.1f, -100.0f, 100.0f);
	ImGui::End();
}


