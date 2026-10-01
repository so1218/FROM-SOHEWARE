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
        // ★ 今回は全てFloatingタイプで生成
        auto enemy = std::make_unique<Enemy>(engine_, i, EnemyType::Floating, managerGroupName_);
        enemy->SetManager(this->GetManager());
        enemy->Initialize();
        enemies_.push_back(std::move(enemy));
    }

    RebuildMaterialSharing(); // ★別関数に切り出し
}

void EnemyManager::RebuildMaterialSharing()
{
    if (enemies_.empty()) return;

    // Typeごとの代表モデルを保持
    std::unordered_map<EnemyType, FE::Model*> archetypeModels;

    for (auto& enemy : enemies_)
    {
        EnemyType type = enemy->GetType();

        if (archetypeModels.find(type) == archetypeModels.end())
        {
            // そのタイプの1体目ならユニーク化して登録
            enemy->GetModel()->MakeMaterialUnique();
            archetypeModels[type] = enemy->GetModel();
        }
        else
        {
            // 2体目以降なら代表モデルから共有
            enemy->GetModel()->ShareMaterialsFrom(archetypeModels[type]);
        }
    }

    // UI用（とりあえずFloatingのモデルをバインド）
    binder_->BindModel("sharedEnemyModel", archetypeModels[EnemyType::Floating]);
}

void EnemyManager::AddEnemy()
{
    int newIndex = static_cast<int>(enemies_.size());

    // ★ ここで追加したいタイプを指定できる
    auto newEnemy = std::make_unique<Enemy>(engine_, newIndex, EnemyType::Floating, managerGroupName_);
    newEnemy->SetManager(this->GetManager());
    newEnemy->Initialize();

    enemies_.push_back(std::move(newEnemy));
    enemyCount_ = static_cast<int>(enemies_.size());

    // マテリアル共有ツリーを再構築
    RebuildMaterialSharing();
}

void EnemyManager::Update()
{
    for (auto& enemy : enemies_) enemy->Update();
}

void EnemyManager::Draw()
{
    for (auto& enemy : enemies_) enemy->Draw();
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