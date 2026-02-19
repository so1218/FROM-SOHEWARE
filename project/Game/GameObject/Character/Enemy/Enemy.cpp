#include "Enemy.h"
#include "CollisionConfig.h"
#include "ImGuiManager.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "AudioPlayer.h"

Enemy::Enemy(Engine* engine) : GameObject(engine, 10)
{
	SetTag("Enemy");
}

Enemy::~Enemy()
{

}

void Enemy::Initialize()
{
	// 衝突属性を設定
	SetCollisionAttribute(kCollisionAttributeEnemy);
	// 衝突対象を自分の属性以外に設定
	SetCollisionMask(kCollisionAttributePlayer);
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
#ifdef IS_DEVELOPMENT
	ImGui::Begin("敵");

	ImGui::End();
#endif
}

