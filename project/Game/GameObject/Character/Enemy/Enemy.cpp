#include "pch.h"
#include "Enemy.h"
#include "CollisionConfig.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "GameDefine.h"

using namespace FE;

Enemy::Enemy(Engine* engine) : GameObject()
{
	SetTag(ObjectTag::Enemy);

	engine_ = engine;

	collider_ = std::make_unique<FE::Collider>(this);
}

Enemy::~Enemy()
{

}

void Enemy::Initialize()
{
	// 衝突属性を設定
	collider_->SetCollisionAttribute(kCollisionAttributeEnemy);
	// 衝突対象を自分の属性以外に設定
	collider_->SetCollisionMask(kCollisionAttributePlayer);
}

void Enemy::Update()
{
	
}

void Enemy::Draw()
{
	
}

void Enemy::OnCollisionEnter(FE::Collider* mine, FE::Collider* other)
{
	// 相手の親を取得
	FE::GameObject* hitObject = other->GetOwner();
	if (!hitObject) return;

	if (mine == collider_.get())
	{
		/*if (auto* player = dynamic_cast<Player*>(hitObject))
		{
			float damage = player->GetAttackPower();
			hp_ -= damage;
		}*/
	}
}

// デバッグ描画処理
void Enemy::DebugDraw()
{
#ifdef IS_DEVELOPMENT
	ImGui::Begin("敵");

	ImGui::End();
#endif
}

