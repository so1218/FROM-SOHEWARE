#include "pch.h"
#include "EnemyManager.h"

using namespace FE;

EnemyManager::EnemyManager(Engine* engine, const std::string& groupName)
    : engine_(engine), managerGroupName_(groupName)
{}

void EnemyManager::Initialize()
{
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, managerGroupName_);
    binder_->Bind("EnemyCount", &enemyCount_, 3);
    enemies_.clear();

    for (int i = 0; i < enemyCount_; ++i)
    {
        auto enemy = std::make_unique<Enemy>(engine_, i, EnemyType::Floating, managerGroupName_);
        enemy->SetManager(this->GetManager());
        enemy->Initialize();
        enemies_.push_back(std::move(enemy));
    }

    RebuildMaterialSharing();
}

void EnemyManager::RebuildMaterialSharing()
{
    if (enemies_.empty()) return;

    std::unordered_map<EnemyType, FE::Model*> archetypeModels;

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
    int newIndex = static_cast<int>(enemies_.size());

    auto newEnemy = std::make_unique<Enemy>(engine_, newIndex, EnemyType::Floating, managerGroupName_);
    newEnemy->SetManager(this->GetManager());
    newEnemy->Initialize();

    enemies_.push_back(std::move(newEnemy));
    enemyCount_ = static_cast<int>(enemies_.size());

    RebuildMaterialSharing();
}

void EnemyManager::Update()
{
    // 死亡した敵のクリーンアップ
    enemies_.erase(
        std::remove_if(enemies_.begin(), enemies_.end(),
            [](const std::unique_ptr<Enemy>& enemy) {
                return !enemy || !enemy->IsActive();
            }),
        enemies_.end()
    );

    enemyCount_ = static_cast<int>(enemies_.size());

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

void EnemyManager::DebugDraw()
{
#ifdef ENABLE_IMGUI
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
    }

    ImGui::Separator();

    for (auto& enemy : enemies_)
    {
        enemy->DebugDraw();
    }

    ImGui::End();
#endif
}