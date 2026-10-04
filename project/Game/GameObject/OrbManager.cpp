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

    binder_->Bind("SharedScale", &sharedScale_, { 1.0f, 1.0f, 1.0f });
    binder_->BindColor("SharedLightColor", &sharedLightColor_, { 0.2f, 0.6f, 1.0f, 1.0f });
    binder_->Bind("TiltAngle", &tiltAngle_, 15.0f, 0.5f, -90.0f, 90.0f);
    binder_->Bind("RotationSpeed", &rotationSpeed_, 2.0f, 0.1f, -20.0f, 20.0f);

    // マスターモデルを作成し、独立したマテリアルを持たせる
    sharedModel_ = std::make_unique<Model>(engine_, "bullet");
    sharedModel_->MakeMaterialUnique();
    binder_->BindModel("SharedOrbModel", sharedModel_.get());

    sharedBubbleModel_ = std::make_unique<Model>(engine_, "sphere");
    sharedBubbleModel_->MakeMaterialUnique();

    auto bubbleMat = sharedBubbleModel_->GetMaterialData();
    if (bubbleMat)
    {
        bubbleMat->isBubble = true;
        binder_->Bind("wobbleAmplitude", &bubbleMat->wobbleAmplitude, 1.0f);
        binder_->Bind("wobbleSpeed", &bubbleMat->wobbleSpeed, 5.0f);
        binder_->Bind("fresnelExponent", &bubbleMat->fresnelExponent, 2.0f);
        binder_->Bind("rainbowIntensity", &bubbleMat->rainbowIntensity, 1.0f);
    }
    binder_->BindModel("SharedBubbleModel", sharedBubbleModel_.get());

    orbs_.clear();

    for (int i = 0; i < orbCount_; ++i)
    {
        auto orb = std::make_unique<Orb>(engine_, i, managerGroupName_);
        orb->SetManager(this->GetManager());
        orb->Initialize();

        orb->GetModel()->ShareMaterialsFrom(sharedModel_.get());
        if (orb->GetBubbleModel())
        {
            orb->GetBubbleModel()->SetBlendMode(BlendMode::kBlendModeNormal);
            orb->GetBubbleModel()->ShareMaterialsFrom(sharedBubbleModel_.get());
        }

        orbs_.push_back(std::move(orb));
    }
}

void OrbManager::Update()
{
    for (auto& orb : orbs_)
    {
        orb->Update(sharedScale_, sharedBubbleModel_->GetTransform().scale_, sharedLightColor_, tiltAngle_, rotationSpeed_);
    }
}

void OrbManager::Draw()
{
    for (auto& orb : orbs_)
    {
        orb->Draw();
    }
}

void OrbManager::AddOrb()
{
    int newIndex = static_cast<int>(orbs_.size());
    auto newOrb = std::make_unique<Orb>(engine_, newIndex, managerGroupName_);
    newOrb->SetManager(this->GetManager());
    newOrb->Initialize();
    newOrb->GetModel()->ShareMaterialsFrom(sharedModel_.get());

    orbs_.push_back(std::move(newOrb));

    orbCount_ = static_cast<int>(orbs_.size());
}

void OrbManager::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::Begin("オーブマネージャー");
    binder_->Draw("OrbCount", "オーブの数");

    if (ImGui::Button("新しいオーブを追加"))
    {
        AddOrb();
    }

    ImGui::Separator();

    ImGui::Text("共通設定");
    binder_->Draw("SharedScale", "共通スケール");
    binder_->Draw("SharedLightColor", "共通ライトカラー");
    binder_->Draw("TiltAngle", "傾き角度");
    binder_->Draw("RotationSpeed", "回転速度");

    ImGui::Separator();

    ImGui::Text("共通バブルシェーダー設定");
    binder_->Draw("wobbleAmplitude", "動く距離");
    binder_->Draw("wobbleSpeed", "波打つ速さ");
    binder_->Draw("fresnelExponent", "グラデーション");
    binder_->Draw("rainbowIntensity", "虹色の輝度");

    ImGui::Separator();

    if (!orbs_.empty())
    {
        binder_->DrawModel("SharedOrbModel", "オーブインスペクター");
        binder_->DrawModel("SharedBubbleModel", "バブルインスペクター");

        for (auto& orb : orbs_)
        {
            orb->GetModel()->ShareMaterialsFrom(sharedModel_.get());
            if (orb->GetBubbleModel())
            {
                orb->GetBubbleModel()->ShareMaterialsFrom(sharedBubbleModel_.get());
            }
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