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
#include "Camera.h"

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
// 浮遊する敵の出す場所を探す回数
constexpr int kFloatingSpawnAttemptCount = 16;
// 浮遊する敵は、カメラの正面からこの角度 (度) より外の方向に出す
constexpr float kFloatingSpawnHiddenAngle = 70.0f;
// 飛んできた機体が最初に向かう、プレイヤーからの距離
constexpr float kArrivalTargetMinDistance = 30.0f;
constexpr float kArrivalTargetMaxDistance = 70.0f;

}

using namespace FS;

EnemyManager::EnemyManager(Engine* engine, const std::string& groupName, const Terrain* terrain, const TreeField* treeField, CoinManager* coinManager)
    : engine_(engine), managerGroupName_(groupName), terrain_(terrain), treeField_(treeField), coinManager_(coinManager)
{}

void EnemyManager::Initialize()
{
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, managerGroupName_);

    if (!thrusterModel_)
    {
        thrusterModel_ = std::make_unique<FS::Model>(engine_, "enemyThruster");
        thrusterModel_->MakeMaterialUnique();
    }
    binder_->BindModel("ThrusterModel", thrusterModel_.get());

    if (!floatingMaterialSource_)
    {
        floatingMaterialSource_ = std::make_unique<FS::Model>(engine_, "Enemy");
        floatingMaterialSource_->MakeMaterialUnique();
    }
    binder_->BindModel("sharedEnemyModel", floatingMaterialSource_.get());

    floatingBinder_ = std::make_unique<FS::PropertyBinder>(engine_, "FloatingEnemy");
    floatingConfig_.Bind(*floatingBinder_);

    zombieBinder_ = std::make_unique<FS::PropertyBinder>(engine_, "ZombieEnemy");
    zombieConfig_.Bind(*zombieBinder_);

    // モデルが無いと作るときにエラーになるので、置いてあるときだけ出す
    const std::vector<std::string> modelNames = ModelManager::GetInstance().GetLoadedModelNames();
    const bool hasMesh = std::find(modelNames.begin(), modelNames.end(), ZombieBehavior::kMeshName) != modelNames.end();
    const bool hasWalk = AnimationManager::GetInstance()->Get(ZombieBehavior::kWalkAnimation) != nullptr;
    hasZombieAssets_ = hasMesh && hasWalk;

    if (hasZombieAssets_ && !zombieMaterialSource_)
    {
        zombieMaterialSource_ = std::make_unique<AnimationModel>(engine_, ZombieBehavior::kMeshName, ZombieBehavior::kWalkAnimation);
        zombieBinder_->BindAnimationModel("ZombieModel", zombieMaterialSource_.get());
    }

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
    floatingSpawnTimer_ = 0.0f;

    // 遊び始めから、フィールドのどこかに上限まで飛ばしておく
    Player* player = GetManager()->FindObjectWithTag<Player>(ObjectTag::Player);
    const Vector3 playerPosition = player ? player->GetTransform().translation_ : Vector3(0.0f, 0.0f, 0.0f);
    if (floatingConfig_.isSpawnEnabled)
    {
        SpawnInitialFloating(playerPosition);
    }
}

int EnemyManager::CountAliveFloating() const
{
    return static_cast<int>(std::count_if(enemies_.begin(), enemies_.end(),
        [](const std::unique_ptr<Enemy>& enemy) { return enemy->IsActive() && enemy->GetType() == EnemyType::Floating; }));
}

float EnemyManager::GetGroundHeight(float x, float z) const
{
    float groundHeight = 0.0f;
    if (terrain_ && terrain_->GetHeightAt(x, z, groundHeight))
    {
        return groundHeight;
    }
    return 0.0f;
}

bool EnemyManager::FindFieldPointAround(const Vector3& origin, float minDistance, float maxDistance, Vector3& outPoint)
{
    if (!terrain_)
    {
        return false;
    }

    const Vector2 center = terrain_->GetCenterXZ();
    const Vector2 halfSize = terrain_->GetSizeXZ() * 0.5f - Vector2(kZombieSpawnEdgeMargin, kZombieSpawnEdgeMargin);
    std::uniform_real_distribution<float> angleRange(0.0f, Math::PI * 2.0f);
    std::uniform_real_distribution<float> distanceRange(std::min(minDistance, maxDistance), std::max(minDistance, maxDistance));

    for (int attempt = 0; attempt < kFloatingSpawnAttemptCount; ++attempt)
    {
        const float angle = angleRange(random_);
        const float distance = distanceRange(random_);
        const Vector3 candidate = { origin.x + std::cos(angle) * distance, 0.0f, origin.z + std::sin(angle) * distance };
        if (std::abs(candidate.x - center.x) <= halfSize.x && std::abs(candidate.z - center.y) <= halfSize.y)
        {
            outPoint = candidate;
            return true;
        }
    }
    return false;
}

void EnemyManager::SpawnInitialFloating(const Vector3& playerPosition)
{
    if (!terrain_)
    {
        return;
    }

    // フィールドの広さの半分までの所に、プレイヤーから離して散らす
    const Vector2 fieldSize = terrain_->GetSizeXZ();
    const float maxDistance = std::max(fieldSize.x, fieldSize.y) * 0.5f;
    for (int count = 0; count < floatingConfig_.maxAliveCount; ++count)
    {
        Vector3 position;
        if (!FindFieldPointAround(playerPosition, floatingConfig_.initialMinDistance, maxDistance, position))
        {
            continue;
        }
        position.y = GetGroundHeight(position.x, position.z) + floatingConfig_.wanderHeight;

        Vector3 firstTarget;
        if (!FindFieldPointAround(position, floatingConfig_.wanderMinStep, floatingConfig_.wanderMaxStep, firstTarget))
        {
            firstTarget = position;
        }
        SpawnFloating(position, firstTarget);
    }
}

bool EnemyManager::SpawnArrivingFloating(Player* player)
{
    const Vector3 playerPosition = player->GetTransform().translation_;

    // 目の前に急に出ると理不尽なので、カメラに映っていない方向を選ぶ
    Vector3 cameraForward = { 0.0f, 0.0f, 1.0f };
    if (Camera* camera = player->GetCamera())
    {
        cameraForward = camera->GetWorldTransform().rotation_.RotateVector({ 0.0f, 0.0f, 1.0f });
    }
    cameraForward.y = 0.0f;
    const bool hasCameraForward = cameraForward.LengthSq() > 0.0001f;
    if (hasCameraForward)
    {
        cameraForward = cameraForward.Normalize();
    }
    const float hiddenCos = std::cos(kFloatingSpawnHiddenAngle * Math::PI / 180.0f);

    for (int attempt = 0; attempt < kFloatingSpawnAttemptCount; ++attempt)
    {
        Vector3 position;
        if (!FindFieldPointAround(playerPosition, floatingConfig_.spawnMinDistance, floatingConfig_.spawnMaxDistance, position))
        {
            return false;
        }

        if (hasCameraForward)
        {
            const Vector3 toSpawn = Vector3(position.x - playerPosition.x, 0.0f, position.z - playerPosition.z).Normalize();
            if (toSpawn.x * cameraForward.x + toSpawn.z * cameraForward.z > hiddenCos)
            {
                continue;
            }
        }

        // 高い所に出して、プレイヤーのいる辺りへ降りながら入ってくる
        position.y = GetGroundHeight(position.x, position.z) + floatingConfig_.spawnHeight;
        Vector3 firstTarget;
        if (!FindFieldPointAround(playerPosition, kArrivalTargetMinDistance, kArrivalTargetMaxDistance, firstTarget))
        {
            firstTarget = playerPosition;
        }
        SpawnFloating(position, firstTarget);
        return true;
    }
    return false;
}

void EnemyManager::SpawnFloating(const Vector3& position, const Vector3& firstWanderTarget)
{
    // 生きている機体と重ならない番号を使い回し、保存グループが増え続けないようにする
    int slot = 0;
    while (std::any_of(enemies_.begin(), enemies_.end(), [&](const std::unique_ptr<Enemy>& enemy)
        { return enemy->IsActive() && enemy->GetType() == EnemyType::Floating && enemy->GetId() == slot; }))
    {
        ++slot;
    }

    auto behavior = std::make_unique<FloatingBehavior>(floatingConfig_, terrain_, treeField_, thrusterModel_.get(), position, firstWanderTarget);
    auto enemy = std::make_unique<Enemy>(engine_, slot, EnemyType::Floating, std::move(behavior), managerGroupName_);
    enemy->AttachToManager(GetManager(), 0);
    enemy->Initialize();
    enemy->GetModel()->ShareMaterialsFrom(floatingMaterialSource_.get());
    enemies_.push_back(std::move(enemy));
}

void EnemyManager::UpdateFloatingSpawning()
{
    Player* player = GetManager()->FindObjectWithTag<Player>(ObjectTag::Player);
    if (!player || !player->IsControllable() || !floatingConfig_.isSpawnEnabled)
    {
        return;
    }

    // 上限まで減っている間だけ数え、倒してすぐには次が来ないようにする
    if (CountAliveFloating() >= floatingConfig_.maxAliveCount)
    {
        floatingSpawnTimer_ = 0.0f;
        return;
    }

    floatingSpawnTimer_ += TimeManager::GetInstance()->GetDeltaTime();
    if (floatingSpawnTimer_ < floatingConfig_.spawnInterval)
    {
        return;
    }

    if (SpawnArrivingFloating(player))
    {
        floatingSpawnTimer_ = 0.0f;
    }
}

void EnemyManager::Update()
{
    // 死亡した敵のクリーンアップ
    for (const auto& enemy : enemies_)
    {
        if (enemy && !enemy->IsActive() && enemy->IsDefeated() && coinManager_)
        {
            const int coinCount = enemy->GetType() == EnemyType::Floating
                ? coinManager_->GetFloatingCoinCount()
                : coinManager_->GetZombieCoinCount();
            coinManager_->SpawnBurst(enemy->GetTransform().translation_, coinCount);
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
    UpdateFloatingSpawning();
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

    auto behavior = std::make_unique<ZombieBehavior>(zombieConfig_, terrain_, zombieMaterialSource_.get(), position, yaw);
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

    // 保存はしない
    if (Player* player = GetManager()->FindObjectWithTag<Player>(ObjectTag::Player))
    {
        bool isIgnoredByEnemies = player->IsIgnoredByEnemies();
        if (ImGui::Checkbox("敵が襲ってこない", &isIgnoredByEnemies))
        {
            player->SetIgnoredByEnemies(isIgnoredByEnemies);
        }
    }

    binder_->DrawModel("sharedEnemyModel", "浮遊する敵のマテリアル (全機で共有)");
    binder_->DrawModel("ThrusterModel", "スラスターのモデル");

    if (ImGui::CollapsingHeader("浮遊している敵共通"))
    {
        ImGui::Text("出ている数: %d / 次が来るまで: %.0f 秒", CountAliveFloating(),
            std::max(floatingConfig_.spawnInterval - floatingSpawnTimer_, 0.0f));
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

        // 位置・回転・スケールは使わない。大きさは「本体」のスケールで変える
        if (zombieMaterialSource_)
        {
            zombieBinder_->DrawAnimationModel("ZombieModel", "ゾンビのマテリアル (全員で共有)");
        }
    }

    ImGui::Separator();

    for (auto& enemy : enemies_)
    {
        enemy->DebugGui();
    }

    ImGui::End();
}