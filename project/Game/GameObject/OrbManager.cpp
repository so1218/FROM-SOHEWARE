#include "pch.h"
#include "OrbManager.h"
#include "TimeManager.h"

using namespace FE;

OrbManager::OrbManager(Engine* engine, const std::string& groupName)
    : engine_(engine), managerGroupName_(groupName)
{}

void OrbManager::Initialize()
{
    binder_ = std::make_unique<PropertyBinder>(engine_, managerGroupName_);

    binder_->Bind("OrbCount", &orbCount_, 3);
    binder_->Bind("RainbowSpeed", &rainbowSpeed_, 0.5f);

    // マスターモデルを作成し、独立したマテリアルを持たせる
    sharedModel_ = std::make_unique<Model>(engine_, "sphere");
    sharedModel_->MakeMaterialUnique();

    // インスペクターにマスターモデルをバインド
    binder_->BindModel("SharedOrbModel", sharedModel_.get());

    orbs_.clear();

    for (int i = 0; i < orbCount_; ++i)
    {
        auto orb = std::make_unique<Orb>(engine_, i, managerGroupName_);
        orb->SetManager(this->GetManager());
        orb->Initialize();

        // ▼生成したオーブはすべてマスターからマテリアルを共有
        orb->GetModel()->ShareMaterialsFrom(sharedModel_.get());

        orbs_.push_back(std::move(orb));
    }
}

void OrbManager::Update()
{
    for (auto& orb : orbs_) orb->Update();

    if (!orbs_.empty())
    {
        time_ += TimeManager::GetInstance()->GetDeltaTime() * rainbowSpeed_;

        Vector4 rainbowColor = Math::HSVToRGB(time_, 1.0f, 1.0f, 1.0f);

        auto* matData = sharedModel_->GetMaterialData();
        if (matData)
        {
            matData->rimColor = { rainbowColor.x, rainbowColor.y, rainbowColor.z };
        }
    }
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
    newOrb->GetModel()->ShareMaterialsFrom(sharedModel_.get());

    orbs_.push_back(std::move(newOrb));

    orbCount_ = static_cast<int>(orbs_.size()); // 数を更新
}

void OrbManager::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("オーブマネージャー");
    binder_->Draw("OrbCount", "オーブの数");
    binder_->Draw("RainbowSpeed", "虹色の遷移スピード");

    if (ImGui::Button("新しいオーブを追加"))
    {
        AddOrb();
    }

    ImGui::Separator();

    if (!orbs_.empty())
    {
        binder_->DrawModel("SharedOrbModel", "オーブ共通モデル＆マテリアル設定");

        for (auto& orb : orbs_)
        {
            orb->GetModel()->ShareMaterialsFrom(sharedModel_.get());
        }
    }
    ImGui::Separator();

    for (auto& orb : orbs_)
    {
        orb->DebugDraw();
    }

    ImGui::End();
#endif
}