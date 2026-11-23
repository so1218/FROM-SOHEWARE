#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "Enemy.h"
#include "PropertyBinder.h"
#include "FloatingBehavior.h"
#include "ZombieBehavior.h"

class CoinManager;
class Player;

class EnemyManager : public FS::GameObject
{
public:
    EnemyManager(FS::Engine* engine, const std::string& groupName, const FS::Terrain* terrain, CoinManager* coinManager);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // 全部消して、遊び始めの状態に戻す
    void ResetEnemies();

private:
    void SpawnEnemies();

    // 浮遊する敵
    void UpdateFloatingSpawning();
    // 遊び始めに、プレイヤーから離れたフィールドの上に置く
    void SpawnInitialFloating(const FS::Vector3& playerPosition);
    // 画面に映っていない遠くの上空に出し、プレイヤーのいる辺りへ向かわせる
    bool SpawnArrivingFloating(Player* player);
    void SpawnFloating(const FS::Vector3& position, const FS::Vector3& firstWanderTarget);
    // プレイヤーの周りで、歩ける地形の内側の点を探す
    bool FindFieldPointAround(const FS::Vector3& origin, float minDistance, float maxDistance, FS::Vector3& outPoint);
    int CountAliveFloating() const;
    float GetGroundHeight(float x, float z) const;

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
    // 全機の本体がマテリアルを共有する元のモデル
    std::unique_ptr<FS::Model> floatingMaterialSource_;

    std::vector<std::unique_ptr<Enemy>> enemies_;

    std::unique_ptr<FS::PropertyBinder> binder_;
    // 共通のパラメータ
    FloatingEnemyConfig floatingConfig_;
    std::unique_ptr<FS::PropertyBinder> floatingBinder_;

    float floatingSpawnTimer_ = 0.0f;

    // ゾンビ
    ZombieConfig zombieConfig_;
    std::unique_ptr<FS::PropertyBinder> zombieBinder_;
    // 全員で共有するマテリアルを持つモデル。描かずに ImGui での調整だけに使う
    std::unique_ptr<FS::AnimationModel> zombieMaterialSource_;
    float zombieSpawnTimer_ = 0.0f;
    bool hasZombieAssets_ = false;
    std::mt19937 random_{ std::random_device{}() };
};