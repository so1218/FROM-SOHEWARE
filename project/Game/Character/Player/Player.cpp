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

#include <numbers>
#include <algorithm>

Player::Player()
{

}

void Player::Initialize(Engine* engine, Camera* camera)
{
	engine_ = engine;
	camera_ = camera;

	modelPlayer_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(ModelID::cube)));

	size_ = { 1.0f, 1.0f, 1.0f };

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

void Player::Update()
{

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

}

// デバッグ描画処理
void Player::DebugDraw()
{
	ImGui::Begin("プレイヤー");
	ImGui::DragFloat3("Transform Translation", &modelPlayer_->GetTransform().translation_.x, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("Transform Scale", &modelPlayer_->GetTransform().scale_.x, 0.1f, -100.0f, 100.0f);
	ImGui::End();
}


