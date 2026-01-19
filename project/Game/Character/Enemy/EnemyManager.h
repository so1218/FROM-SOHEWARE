#pragma once
#include "Enemy.h"
#include <memory>
#include "GameObjectManager.h"

class EnemyManager
{
public:
	EnemyManager(Engine* engine, Player* player, GameObjectManager* objectManager);

	void Update();

	// 敵を生成する関数
	void SpawnEnemy(const EnemyData& data, const Vector3& positon);
	void Reset();

private:
	Engine* engine_;
	Player* player_;

	float spawnTimer_ = 0.0f;
	float spawnInterval_ = 1.4f; 
	float spawnRadius_ = 30.0f;

	GameObjectManager* objectManager_;
};