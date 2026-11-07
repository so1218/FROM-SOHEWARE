#include "pch.h"
#include "EnemyManager.h"
#include "CoinManager.h"

using namespace FS;

EnemyManager::EnemyManager(Engine* engine, const std::string& groupName)
    : engine_(engine), managerGroupName_(groupName)
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

    ImGui::Separator();

    for (auto& enemy : enemies_)
    {
        enemy->DebugGui();
    }

    ImGui::End();
}