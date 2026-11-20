#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "Enemy.h"
#include "PropertyBinder.h"
#include "FloatingBehavior.h"
#include "ZombieBehavior.h"

class CoinManager;

class EnemyManager : public FS::GameObject
{
public:
    EnemyManager(FS::Engine* engine, const std::string& groupName, const FS::Terrain* terrain, CoinManager* coinManager);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // 敵の種類ごとにマテリアルを共有する関数
    void RebuildMaterialSharing();
    void AddEnemy(); 
    // 保存されている数だけ作り直す
    void ResetEnemies();

private:
    std::unique_ptr<Enemy> CreateEnemy(int index);
    void SpawnEnemies();

    // ゾンビ
    void UpdateZombieSpawning();
    // プレイヤーの周りの地形の上で、ほかのゾンビと重ならない場所を探す
    bool FindZombieSpawnPosition(const FS::Vector3& playerPosition, FS::Vector3& outPosition);
    void SpawnZombie(const FS::Vector3& position, const FS::Vector3& playerPosition);
    // ゾンビどうしが重ならないように押し離す
    void SeparateZombies();
    int CountAliveZombies() const;

    FS::Engine* engine_;
    std::string managerGroupName_;
    const FS::Terrain* terrain_ = nullptr;
    CoinManager* coinManager_ = nullptr;

    // 全機のスラスターがマテリアルを共有する元のモデル
    std::unique_ptr<FS::Model> thrusterModel_;

    std::vector<std::unique_ptr<Enemy>> enemies_;

    std::unique_ptr<FS::PropertyBinder> binder_;
    // 共通のパラメータ
    FloatingEnemyConfig floatingConfig_;
    std::unique_ptr<FS::PropertyBinder> floatingBinder_;

    int enemyCount_ = 0; // JSONに保存される全体の数

    // ゾンビ
    ZombieConfig zombieConfig_;
    std::unique_ptr<FS::PropertyBinder> zombieBinder_;
    float zombieSpawnTimer_ = 0.0f;
    bool hasZombieAssets_ = false;
    std::mt19937 random_{ std::random_device{}() };
};