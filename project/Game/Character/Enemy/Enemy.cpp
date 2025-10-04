#include "Enemy.h"
#include "CollisionConfig.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "GlobalVariables.h"

Enemy::Enemy()
{

}

void Enemy::Initialize(Engine* engine, Camera* camera)
{
	engine_ = engine;
	camera_ = camera;

	modelEnemy_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(ModelID::sphere)));

	size_ = { 1.0f, 1.0f, 1.0f };

	SetRadius(size_.x); // 半径を設定
	// 衝突属性を設定
	SetCollisionAttribute(kCollisionAttributeEnemy);
	// 衝突対象を自分の属性以外に設定
	SetCollisionMask(kCollisionAttributePlayer);

	// グループ名を追加
	GlobalVariables::GetInstance()->CreateGroup(GetGlobalVariableGroupName());
	GlobalVariables::GetInstance()->LoadFiles();
}

void Enemy::ApplyGlobalVariables()
{

}


void Enemy::Update()
{

}

void Enemy::Draw()
{
	
}


// デバッグ描画処理
void Enemy::DebugDraw()
{
	ImGui::Begin("敵");

	ImGui::End();
}


Vector3 Enemy::GetWorldPosition()
{
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得(ワールド座標)
	worldPos.x = modelEnemy_->GetTransform().matWorld_.m[3][0];
	worldPos.y = modelEnemy_->GetTransform().matWorld_.m[3][1];
	worldPos.z = modelEnemy_->GetTransform().matWorld_.m[3][2];

	return worldPos;
}

void Enemy::UpdateAABB()
{
	Vector3 center = modelEnemy_->GetTransform().GetWorldPosition(); // プレイヤーの基準位置
	float halfW = size_.x / 2.0f;
	float halfH = size_.y / 2.0f;
	float halfD = size_.z / 2.0f;

	aabb_.min = { center.x - halfW, center.y - halfH, center.z - halfD };
	aabb_.max = { center.x + halfW, center.y + halfH, center.z + halfD };
}

void Enemy::OnCollision()
{

}