#include "EnemyManager.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "Player.h"

EnemyManager::EnemyManager(Engine* engine, Player* player, GameObjectManager* objectManager)
    : engine_(engine),
    player_(player),
    objectManager_(objectManager)
{
}

void EnemyManager::Update()
{
    // 敵のスポーン処理
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    spawnTimer_ += deltaTime;

    if (spawnTimer_ >= spawnInterval_)
    {
        spawnTimer_ -= spawnInterval_;

        if (Enemy::GetEnemyCount() >= 180)
        {
            return;
        }

        Vector3 playerPos = player_->GetWorldPosition();
        float randomAngle = Math::RandomFloat(0.0f, 2.0f * Math::PI);

        Vector3 spawnPos;
        spawnPos.x = playerPos.x + std::cos(randomAngle) * spawnRadius_;
        spawnPos.y = 0.0f;
        spawnPos.z = playerPos.z + std::sin(randomAngle) * spawnRadius_;

        EnemyData enemyData;
        enemyData.modelId = ModelID::enemy;
        enemyData.hp = 50.0f;
        enemyData.speed = 3.0f;
        enemyData.size = { 1.0f, 1.0f, 1.0f };

        SpawnEnemy(enemyData, spawnPos);
    }
}

void EnemyManager::SpawnEnemy(const EnemyData& data, const Vector3& position)
{
    // 敵の生成
    auto newEnemy = std::make_unique<Enemy>(engine_, player_, objectManager_, data);

    // 初期位置設定
    WorldTransform& transform = newEnemy->GetWorldTransform();
    transform.translation_ = position;

    transform.UpdateMatrix();

    // 初期化処理
    newEnemy->Initialize();

    // オブジェクトマネージャーに登録
    objectManager_->AddObject(std::move(newEnemy));
}

void EnemyManager::Reset()
{
    spawnTimer_ = 0.0f;
}