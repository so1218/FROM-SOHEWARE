#include "pch.h"
#include "OrbManager.h"

using namespace FE;

OrbManager::OrbManager(Engine* engine, const std::string& groupName)
    : engine_(engine), managerGroupName_(groupName)
{}

void OrbManager::Initialize()
{
    binder_ = std::make_unique<PropertyBinder>(engine_, managerGroupName_);

    binder_->Bind("OrbCount", &orbCount_, 3);

    orbs_.clear();

    for (int i = 0; i < orbCount_; ++i)
    {
        auto orb = std::make_unique<Orb>(engine_, i, managerGroupName_);

        orb->SetManager(this->GetManager());

        orb->Initialize(); 
        orbs_.push_back(std::move(orb));
    }
}

void OrbManager::Update()
{
    for (auto& orb : orbs_) orb->Update();
}

void OrbManager::Draw()
{
    for (auto& orb : orbs_) orb->Draw();
}

void OrbManager::AddOrb()
{
    int newIndex = static_cast<int>(orbs_.size());
    auto newOrb = std::make_unique<Orb>(engine_, newIndex, managerGroupName_);
    newOrb->SetManager(this->GetManager());
    newOrb->Initialize();
    orbs_.push_back(std::move(newOrb));

    orbCount_ = static_cast<int>(orbs_.size()); // 数を更新
}

void OrbManager::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("オーブマネージャー");
    binder_->Draw("OrbCount", "オーブの数");

    if (ImGui::Button("新しいオーブを追加"))
    {
        AddOrb();
    }

    ImGui::Separator();

    // 個別のオーブのインスペクターを表示
    for (auto& orb : orbs_)
    {
        orb->DebugDraw();
    }

    ImGui::End();
#endif
}