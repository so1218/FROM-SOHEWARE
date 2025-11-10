#include "Enemy.h"
#include "CollisionConfig.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "Player.h"
#include "TimeManager.h"
#include "GlobalVariables.h"

Enemy::Enemy(Engine* engine, Camera* camera, Player* player, const EnemyData& data)
{
	engine_ = engine;
	camera_ = camera;
	player_ = player;

	// 設計図(data)からステータスを初期化
	hp_ = data.hp;
	speed_ = data.speed;
	size_ = data.size;
	modelEnemy_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(data.modelId)));
}

void Enemy::Initialize()
{
	SetRadius(size_.x); // 半径を設定
	// 衝突属性を設定
	SetCollisionAttribute(kCollisionAttributeEnemy);
	// 衝突対象を自分の属性以外に設定
	SetCollisionMask(kCollisionAttributePlayer | kCollisionAttributePlayerWeapon);

	// グループ名を追加
	GlobalVariables::GetInstance()->CreateGroup(GetGlobalVariableGroupName());
	GlobalVariables::GetInstance()->LoadFiles();
}

void Enemy::ApplyGlobalVariables()
{

}


void Enemy::Update()
{
	// 死亡していたら何もしない
	if (isDead_) {
		return;
	}

	// --- プレイヤー追跡ロジック ---
	// 1. プレイヤーの座標と自分の座標を取得
	Vector3 playerPos = player_->GetWorldPosition();
	Vector3 selfPos = GetWorldPosition();

	// 2. プレイヤーへの方向ベクトルを計算
	Vector3 direction = playerPos - selfPos;

	direction.y = 0.0f;

	// 4. 方向ベクトルを正規化
	if (direction.Length() > 0.001f) // ゼロ除算を避ける
	{ 
		direction = direction.Normalize();
	}

	// 5. 速度とデルタタイムをかけて、このフレームでの移動量を計算
	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
	Vector3 velocity = direction * speed_ * deltaTime;

	// 6. 座標を更新
	WorldTransform& transform = modelEnemy_->GetTransform();
	transform.translation_.x += velocity.x;
	transform.translation_.y += velocity.y;
	transform.translation_.z += velocity.z;

	// ワールド行列とAABBを更新
	transform.UpdateMatrix();
	UpdateAABB();
}

void Enemy::TakeDamage(float damage)
{
	if (isDead_) return; 

	hp_ -= damage;
	if (hp_ <= 0.0f) 
	{
		isDead_ = true;
	}
}

void Enemy::Draw()
{
	// 死亡していたら描画しない
	if (isDead_) {
		return;
	}
	modelEnemy_->Draw();
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

void Enemy::OnCollision(Collider* other)
{
	// もしプレイヤーにぶつかったら
	if (other->GetCollisionAttribute() & kCollisionAttributePlayer)
	{
		isDead_ = true;
	}
}