#include "pch.h"
#include "EnemyManager.h"

using namespace FE;

EnemyManager::EnemyManager(Engine* engine, const std::string& groupName)
    : engine_(engine), managerGroupName_(groupName)
{}

void EnemyManager::Initialize()
{
    binder_ = std::make_unique<PropertyBinder>(engine_, managerGroupName_);
    binder_->Bind("EnemyCount", &enemyCount_, 3);
    enemies_.clear();

    for (int i = 0; i < enemyCount_; ++i)
    {
        auto enemy = std::make_unique<Enemy>(engine_, i, managerGroupName_);
        enemy->SetManager(this->GetManager());
        enemy->Initialize();
        enemies_.push_back(std::move(enemy));
    }

    if (!enemies_.empty())
    {
        // 1体目のマテリアルを敵グループ専用として独立
        enemies_[0]->GetModel()->MakeMaterialUnique();

        // 2体目以降は、Shareで1体目のマテリアルを共有
        for (size_t i = 1; i < enemies_.size(); ++i)
        {
            enemies_[i]->GetModel()->ShareMaterialsFrom(enemies_[0]->GetModel());
        }

        // マネージャーのインスペクターに代表として1体目のモデルを登録
        binder_->BindModel("sharedEnemyModel", enemies_[0]->GetModel());
    }
}

void EnemyManager::Update()
{
    for (auto& enemy : enemies_) enemy->Update();
}

void EnemyManager::Draw()
{
    for (auto& enemy : enemies_) enemy->Draw();
}

void EnemyManager::AddEnemy()
{
    int newIndex = static_cast<int>(enemies_.size());
    auto newEnemy = std::make_unique<Enemy>(engine_, newIndex, managerGroupName_);
    newEnemy->SetManager(this->GetManager());
    newEnemy->Initialize();

    if (!enemies_.empty())
    {
        newEnemy->GetModel()->ShareMaterialsFrom(enemies_[0]->GetModel());
    }

    enemies_.push_back(std::move(newEnemy));
    enemyCount_ = static_cast<int>(enemies_.size()); // 数を更新
}

void EnemyManager::DebugDraw()
{
#ifdef IS_DEVELOPMENT
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

        for (size_t i = 1; i < enemies_.size(); ++i)
        {
            enemies_[i]->GetModel()->ShareMaterialsFrom(enemies_[0]->GetModel());
        }
    }
    ImGui::Separator();

    for (auto& enemy : enemies_)
    {
        enemy->DebugDraw();
    }

    ImGui::End();
#endif
}