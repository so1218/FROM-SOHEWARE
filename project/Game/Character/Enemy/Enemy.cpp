#include "Enemy.h"
#include "CollisionConfig.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "Player.h"
#include "TimeManager.h"
#include "GlobalVariables.h"
#include "ExperienceGem.h"

Enemy::Enemy(Engine* engine, Camera* camera, Player* player, GameObjectManager* objectManager, const EnemyData& data)
{
	engine_ = engine;
	camera_ = camera;
	player_ = player;
	objectManager_ = objectManager;

	// dataからステータスを初期化
	hp_ = data.hp;
	speed_ = data.speed;
	size_ = data.size;
	modelEnemy_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(data.modelId)));
	modelEnemy_->SetColor(0x000088ff);
	animationEnemy_ = std::make_unique<AnimationModel>(engine_, camera_, *ModelHandle::Get(data.modelId), AnimationHandle::Get(data.animationId));
}

void Enemy::Initialize()
{
	SetRadius(size_.x); // 半径を設定
	// 衝突属性を設定
	SetCollisionAttribute(kCollisionAttributeEnemy);
	// 衝突対象を自分の属性以外に設定
	SetCollisionMask(kCollisionAttributePlayer | kCollisionAttributePlayerWeaponKnife | kCollisionAttributePlayerWeaponAxe);

	// グループ名を追加
	GlobalVariables::GetInstance()->CreateGroup(GetGlobalVariableGroupName());
	GlobalVariables::GetInstance()->LoadFiles();

	ApplyGlobalVariables();
}

void Enemy::ApplyGlobalVariables()
{

}


void Enemy::Update()
{
	// フラッシュタイマーの更新
	if (flashTimer_ > 0)
	{
		flashTimer_--;
	}

	// ノックバック処理
	if (knockbackVelocity_.Length() > 0.001f)
	{
		WorldTransform& transform = modelEnemy_->GetTransform();
		transform.translation_.x += knockbackVelocity_.x;
		transform.translation_.y += knockbackVelocity_.y;
		transform.translation_.z += knockbackVelocity_.z;

		// 摩擦で減速させる
		knockbackVelocity_ *= knockbackFriction_;

		// ある程度小さくなったら0にする
		if (knockbackVelocity_.Length() < 0.01f)
		{
			knockbackVelocity_ = { 0.0f, 0.0f, 0.0f };
		}
	}

	// 通常の追跡ロジック

	Vector3 playerPos = player_->GetWorldPosition();
	Vector3 selfPos = GetWorldPosition();

	Vector3 direction = playerPos - selfPos;
	direction.y = 0.0f;

	WorldTransform& transform = modelEnemy_->GetTransform();

	if (direction.Length() > 0.001f)
	{
		Vector3 forwardDirection = direction.Normalize();
		Vector3 upVector = { 0.0f, 1.0f, 0.0f };
		transform.rotationQuaternion_ = Quaternion::LookRotation(forwardDirection, upVector);

		direction = direction.Normalize();
	}

	float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
	Vector3 velocity = direction * speed_ * deltaTime;

	// 座標を更新
	transform.translation_.x += velocity.x;
	transform.translation_.y += velocity.y;
	transform.translation_.z += velocity.z;

	// ワールド行列とAABBを更新
	transform.UpdateMatrix();
	UpdateAABB();

	animationEnemy_->Update(1.0f, true);
	animationEnemy_->SetTransform(transform);
}

void Enemy::TakeDamage(float damage, const Vector3& hitSourcePosition)
{
	hp_ -= damage;

	// 白フラッシュを開始
	flashTimer_ = kFlashDuration_;

	// ノックバック計算
	// 敵が吹き飛ぶ方向を求める
	Vector3 knockbackDir = GetWorldPosition() - hitSourcePosition;
	knockbackDir.y = 0.0f; // XZ平面のみ

	if (knockbackDir.Length() > 0.001f)
	{
		knockbackDir = knockbackDir.Normalize();
		// 瞬発的な速度を与える
		knockbackVelocity_ = knockbackDir * knockbackPower_;
	}

	if (hp_ <= 0.0f) 
	{
		isDead_ = true;
		// 死亡時に経験値を生成
		SpawnExperienceGem();
	}
}

void Enemy::SpawnExperienceGem()
{
	// 経験値を生成
	auto experience = std::make_unique<ExperienceGem>(engine_, camera_, player_);

	// 敵がいた位置に経験値を配置する
	experience->GetWorldTransform().translation_ = GetWorldPosition();

	// 経験値の初期化
	experience->Initialize();

	objectManager_->AddObject(std::move(experience));
}

void Enemy::Draw()
{
	if (flashTimer_ > 0)
	{

		modelEnemy_->SetColor(0xff0000ff);
	}
	else
	{
		modelEnemy_->SetColor(0x0000ffff);
	}
	modelEnemy_->Draw();
	if (flashTimer_ > 0)
	{
		modelEnemy_->SetColor(0x0000ffff);
	}
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