#include "pch.h"
#include "WaterManager.h"

using namespace FE;

WaterManager::WaterManager(Engine* engine, const std::string& groupName)
    : engine_(engine), groupName_(groupName)
{}

void WaterManager::Initialize()
{
    binder_ = std::make_unique<PropertyBinder>(engine_, groupName_);

    // JSONに保存・復元される全体の数をバインド (初期デフォルト値: 1)
    binder_->Bind("WaterCount", &waterCount_, 1);

    waters_.clear();

    // ロードされた waterCount_ の数だけ WaterObject を生成・初期化
    for (int i = 0; i < waterCount_; ++i)
    {
        auto water = std::make_unique<WaterObject>(engine_, i, groupName_);
        water->Initialize(); // 各 WaterObject 内部で "Water_0", "Water_1" 等の JSON データを読込
        waters_.push_back(std::move(water));
    }
}

void WaterManager::Update()
{
    for (auto& water : waters_)
    {
        water->Update();
    }
}

void WaterManager::Draw()
{
    for (auto& water : waters_)
    {
        water->Draw();
    }
}

void WaterManager::AddWater()
{
    int newIndex = static_cast<int>(waters_.size());
    auto newWater = std::make_unique<WaterObject>(engine_, newIndex, groupName_);
    newWater->Initialize();

    waters_.push_back(std::move(newWater));

    waterCount_ = static_cast<int>(waters_.size()); // 管理数を更新してJSON保存対象にする
}

void WaterManager::RemoveWater(int id)
{
    waters_.erase(
        std::remove_if(waters_.begin(), waters_.end(),
            [id](const std::unique_ptr<WaterObject>& w) { return w->GetId() == id; }),
        waters_.end()
    );

    // IDの歯抜けを防止するため、残ったオブジェクトのIDを0からの連番に振り直す
    for (size_t i = 0; i < waters_.size(); ++i)
    {
        waters_[i]->SetId(static_cast<int>(i)); // groupName の渡し直しが不要に
    }

    waterCount_ = static_cast<int>(waters_.size());
}

void WaterManager::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::Begin("水マネージャー");
    binder_->Draw("WaterCount", "水の数");

    if (ImGui::Button("新しい水を追加"))
    {
        AddWater();
    }

    ImGui::Separator();

    int waterToDelete = -1;

    for (auto& water : waters_)
    {
        water->DebugDraw();

        std::string deleteLabel = "この水を削除 ##" + std::to_string(water->GetId());
        if (ImGui::Button(deleteLabel.c_str()))
        {
            waterToDelete = water->GetId();
        }
        ImGui::Separator();
    }

    ImGui::End();

    if (waterToDelete != -1)
    {
        RemoveWater(waterToDelete);
    }
#endif
}


