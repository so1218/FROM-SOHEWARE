#include "pch.h"
#include "EnemyManager.h"
#include "CoinManager.h"
#include "Player.h"
#include "GameDefine.h"
#include "GameObjectManager.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include "Terrain.h"
#include "TimeManager.h"
#include "MathUtils.h"

namespace
{

// ゾンビの Enemy の番号
constexpr int kZombieIdOffset = 1000;
// ゾンビの保存グループ
constexpr const char* kZombieGroupName = "GameZombie";
// 地形の端からこれより内側に出す
constexpr float kZombieSpawnEdgeMargin = 10.0f;
// 出す場所を探す回数
constexpr int kZombieSpawnAttemptCount = 8;

}

using namespace FS;

EnemyManager::EnemyManager(Engine* engine, const std::string& groupName, const Terrain* terrain, CoinManager* coinManager)
    : engine_(engine), managerGroupName_(groupName), terrain_(terrain), coinManager_(coinManager)
{}

void EnemyManager::Initialize()
{
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, managerGroupName_);
    binder_->Bind("EnemyCount", &enemyCount_, 3);

    if (!thrusterModel_)
    {
        thrusterModel_ = std::make_unique<FS::Model>(engine_, "enemyThruster");
        thrusterModel_->MakeMaterialUnique();
    }
    binder_->BindModel("ThrusterModel", thrusterModel_.get());

    floatingBinder_ = std::make_unique<FS::PropertyBinder>(engine_, "FloatingEnemy");
    floatingConfig_.Bind(*floatingBinder_);

    zombieBinder_ = std::make_unique<FS::PropertyBinder>(engine_, "ZombieEnemy");
    zombieConfig_.Bind(*zombieBinder_);

    // モデルが無いと作るときにエラーになるので、置いてあるときだけ出す
    const std::vector<std::string> modelNames = ModelManager::GetInstance().GetLoadedModelNames();
    const bool hasMesh = std::find(modelNames.begin(), modelNames.end(), ZombieBehavior::kMeshName) != modelNames.end();
    const bool hasWalk = AnimationManager::GetInstance()->Get(ZombieBehavior::kWalkAnimation) != nullptr;
    hasZombieAssets_ = hasMesh && hasWalk;

    SpawnEnemies();
}

void EnemyManager::ResetEnemies()
{
    for (auto& enemy : enemies_)
    {
        if (enemy->IsActive())
        {
            enemy->Despawn();
        }
    }
    SpawnEnemies();
}

void EnemyManager::SpawnEnemies()
{
    enemies_.clear();
    zombieSpawnTimer_ = 0.0f;
    for (int i = 0; i < enemyCount_; ++i)
    {
        enemies_.push_back(CreateEnemy(i));
    }

    RebuildMaterialSharing();
}

void EnemyManager::RebuildMaterialSharing()
{
    if (enemies_.empty()) return;

    std::unordered_map<EnemyType, FS::Model*> archetypeModels;

    for (auto& enemy : enemies_)
    {
        EnemyType type = enemy->GetType();

        if (archetypeModels.find(type) == archetypeModels.end())
        {
            enemy->GetModel()->MakeMaterialUnique();
            archetypeModels[type] = enemy->GetModel();
        }
        else
        {
            enemy->GetModel()->ShareMaterialsFrom(archetypeModels[type]);
        }
    }

    if (archetypeModels.count(EnemyType::Floating))
    {
        binder_->BindModel("sharedEnemyModel", archetypeModels[EnemyType::Floating]);
    }
}

void EnemyManager::AddEnemy()
{
    int newIndex = enemyCount_;

    enemies_.push_back(CreateEnemy(newIndex));
    ++enemyCount_;

    RebuildMaterialSharing();
}

std::unique_ptr<Enemy> EnemyManager::CreateEnemy(int index)
{
    auto behavior = std::make_unique<FloatingBehavior>(floatingConfig_, terrain_, thrusterModel_.get());
    auto enemy = std::make_unique<Enemy>(engine_, index, EnemyType::Floating, std::move(behavior), managerGroupName_);
    enemy->AttachToManager(GetManager(), 0); 
    enemy->Initialize();
    return enemy;
}

void EnemyManager::Update()
{
    // 死亡した敵のクリーンアップ
    for (const auto& enemy : enemies_)
    {
        if (enemy && !enemy->IsActive() && enemy->IsDefeated() && coinManager_)
        {
            coinManager_->SpawnBurst(enemy->GetTransform().translation_);
        }
    }

    enemies_.erase(
        std::remove_if(enemies_.begin(), enemies_.end(),
            [](const std::unique_ptr<Enemy>& enemy) {
                return !enemy || !enemy->IsActive();
            }),
        enemies_.end()
    );

    for (auto& enemy : enemies_)
    {
        if (enemy->IsActive())
        {
            enemy->Update();
        }
    }

    SeparateZombies();
    UpdateZombieSpawning();
}

int EnemyManager::CountAliveZombies() const
{
    return static_cast<int>(std::count_if(enemies_.begin(), enemies_.end(),
        [](const std::unique_ptr<Enemy>& enemy) { return enemy->IsActive() && enemy->GetType() == EnemyType::Zombie; }));
}

void EnemyManager::UpdateZombieSpawning()
{
    Player* player = GetManager()->FindObjectWithTag<Player>(ObjectTag::Player);
    if (!player || !player->IsControllable())
    {
        return;
    }
    const Vector3 playerPosition = player->GetTransform().translation_;

    // 遠く離れたゾンビは消して、プレイヤーの近くに出し直せるようにする
    for (auto& enemy : enemies_)
    {
        if (enemy->IsActive() && enemy->GetType() == EnemyType::Zombie &&
            (enemy->GetTransform().translation_ - playerPosition).Length() > zombieConfig_.despawnDistance)
        {
            enemy->Despawn();
        }
    }

    if (!hasZombieAssets_ || !zombieConfig_.isSpawnEnabled || CountAliveZombies() >= zombieConfig_.maxAliveCount)
    {
        return;
    }

    zombieSpawnTimer_ += TimeManager::GetInstance()->GetDeltaTime();
    if (zombieSpawnTimer_ < zombieConfig_.spawnInterval)
    {
        return;
    }

    Vector3 spawnPosition;
    if (FindZombieSpawnPosition(playerPosition, spawnPosition))
    {
        SpawnZombie(spawnPosition, playerPosition);
        zombieSpawnTimer_ = 0.0f;
    }
}

bool EnemyManager::FindZombieSpawnPosition(const Vector3& playerPosition, Vector3& outPosition)
{
    if (!terrain_)
    {
        return false;
    }

    const Vector2 center = terrain_->GetCenterXZ();
    const Vector2 halfSize = terrain_->GetSizeXZ() * 0.5f - Vector2(kZombieSpawnEdgeMargin, kZombieSpawnEdgeMargin);
    const float minDistance = std::min(zombieConfig_.spawnMinDistance, zombieConfig_.spawnMaxDistance);
    const float maxDistance = std::max(zombieConfig_.spawnMinDistance, zombieConfig_.spawnMaxDistance);
    std::uniform_real_distribution<float> angleRange(0.0f, Math::PI * 2.0f);
    std::uniform_real_distribution<float> distanceRange(minDistance, maxDistance);

    for (int attempt = 0; attempt < kZombieSpawnAttemptCount; ++attempt)
    {
        const float angle = angleRange(random_);
        const float distance = distanceRange(random_);
        const Vector3 candidate = { playerPosition.x + std::cos(angle) * distance, 0.0f, playerPosition.z + std::sin(angle) * distance };

        // 歩ける地形の外には出さない
        if (std::abs(candidate.x - center.x) > halfSize.x || std::abs(candidate.z - center.y) > halfSize.y)
        {
            continue;
        }

        const bool isCrowded = std::any_of(enemies_.begin(), enemies_.end(), [&](const std::unique_ptr<Enemy>& enemy)
            {
                if (!enemy->IsActive() || enemy->GetType() != EnemyType::Zombie) return false;
                const Vector3 offset = enemy->GetTransform().translation_ - candidate;
                return offset.x * offset.x + offset.z * offset.z < zombieConfig_.separationRadius * zombieConfig_.separationRadius * 4.0f;
            });
        if (isCrowded)
        {
            continue;
        }

        outPosition = candidate;
        return true;
    }
    return false;
}

void EnemyManager::SpawnZombie(const Vector3& position, const Vector3& playerPosition)
{
    // 生きているゾンビと重ならない番号を使い回し、保存グループが増え続けないようにする
    int slot = 0;
    while (std::any_of(enemies_.begin(), enemies_.end(), [&](const std::unique_ptr<Enemy>& enemy)
        { return enemy->IsActive() && enemy->GetType() == EnemyType::Zombie && enemy->GetId() == kZombieIdOffset + slot; }))
    {
        ++slot;
    }

    // プレイヤーの方を向いて這い出る
    const float yaw = std::atan2(playerPosition.x - position.x, playerPosition.z - position.z);

    auto behavior = std::make_unique<ZombieBehavior>(zombieConfig_, terrain_, position, yaw);
    auto enemy = std::make_unique<Enemy>(engine_, kZombieIdOffset + slot, EnemyType::Zombie, std::move(behavior), kZombieGroupName);
    enemy->AttachToManager(GetManager(), 0);
    enemy->Initialize();
    enemies_.push_back(std::move(enemy));
}

void EnemyManager::SeparateZombies()
{
    const float minDistance = zombieConfig_.separationRadius;
    if (minDistance <= 0.0f)
    {
        return;
    }

    std::vector<Enemy*> zombies;
    for (auto& enemy : enemies_)
    {
        if (enemy->IsActive() && enemy->GetType() == EnemyType::Zombie)
        {
            zombies.push_back(enemy.get());
        }
    }

    // 重なっている分の半分ずつ、互いに反対へ押す
    for (size_t first = 0; first < zombies.size(); ++first)
    {
        for (size_t second = first + 1; second < zombies.size(); ++second)
        {
            Vector3& firstPosition = zombies[first]->GetTransform().translation_;
            Vector3& secondPosition = zombies[second]->GetTransform().translation_;
            const float offsetX = secondPosition.x - firstPosition.x;
            const float offsetZ = secondPosition.z - firstPosition.z;
            const float distanceSq = offsetX * offsetX + offsetZ * offsetZ;
            if (distanceSq >= minDistance * minDistance || distanceSq < 0.000001f)
            {
                continue;
            }

            const float distance = std::sqrt(distanceSq);
            const float push = (minDistance - distance) * 0.5f / distance;
            firstPosition.x -= offsetX * push;
            firstPosition.z -= offsetZ * push;
            secondPosition.x += offsetX * push;
            secondPosition.z += offsetZ * push;
        }
    }

    for (Enemy* zombie : zombies)
    {
        zombie->SyncTransform();
    }
}

void EnemyManager::Draw()
{
    for (auto& enemy : enemies_)
    {
        if (enemy->IsActive())
        {
            enemy->Draw();
        }
    }
}

void EnemyManager::DebugGui()
{
    ImGui::Begin("敵マネージャー");
    binder_->Draw("EnemyCount", "敵の数");

    if (ImGui::Button("新しい敵を追加"))
    {
        AddEnemy();
    }

    ImGui::Separator();

    if (!enemies_.empty())
    {
        binder_->DrawModel("sharedEnemyModel", "敵共通モデルインスペクター");
        binder_->DrawModel("ThrusterModel", "スラスターのモデル");
    }

    if (ImGui::CollapsingHeader("浮遊している敵共通"))
    {
        floatingConfig_.Draw(*floatingBinder_);
    }

    if (ImGui::CollapsingHeader("ゾンビ共通"))
    {
        if (!hasZombieAssets_)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "%s と %s が無いので出しません", ZombieBehavior::kMeshName, ZombieBehavior::kWalkAnimation);
        }
        ImGui::Text("出ている数: %d", CountAliveZombies());
        zombieConfig_.Draw(*zombieBinder_);
    }

    ImGui::Separator();

    for (auto& enemy : enemies_)
    {
        enemy->DebugGui();
    }

    ImGui::End();
}