#include "pch.h"
#include "AmmoManager.h"
#include "TimeManager.h"

using namespace FE;

AmmoManager::AmmoManager(Engine* engine, const std::string& groupName)
    : engine_(engine), managerGroupName_(groupName)
{}

void AmmoManager::Initialize()
{
    binder_ = std::make_unique<PropertyBinder>(engine_, managerGroupName_);

    binder_->Bind("AmmoCount", &ammoCount_, 3);

    binder_->Bind("SharedScale", &sharedScale_, { 1.0f, 1.0f, 1.0f });
    binder_->BindColor("SharedLightColor", &sharedLightColor_, { 0.2f, 0.6f, 1.0f, 1.0f });
    binder_->Bind("TiltAngle", &tiltAngle_, 15.0f, 0.5f, -90.0f, 90.0f);
    binder_->Bind("RotationSpeed", &rotationSpeed_, 2.0f, 0.1f, -20.0f, 20.0f);

    // マスターモデルを作成し、独立したマテリアルを持たせる
    sharedModel_ = std::make_unique<Model>(engine_, "bullet");
    sharedModel_->MakeMaterialUnique();
    binder_->BindModel("SharedAmmoModel", sharedModel_.get());

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

    ammo_.clear();

    for (int i = 0; i < ammoCount_; ++i)
    {
        auto ammo = std::make_unique<Ammo>(engine_, i, managerGroupName_);
        ammo->SetManager(this->GetManager());
        ammo->Initialize();

        ammo->GetModel()->ShareMaterialsFrom(sharedModel_.get());
        if (ammo->GetBubbleModel())
        {
            ammo->GetBubbleModel()->SetBlendMode(BlendMode::kBlendModeNormal);
            ammo->GetBubbleModel()->ShareMaterialsFrom(sharedBubbleModel_.get());
        }

        ammo_.push_back(std::move(ammo));
    }
}

void AmmoManager::Update()
{
    for (auto& ammo : ammo_)
    {
        ammo->Update(sharedScale_, sharedBubbleModel_->GetTransform().scale_, sharedLightColor_, tiltAngle_, rotationSpeed_);
    }
}

void AmmoManager::Draw()
{
    for (auto& ammo : ammo_)
    {
        ammo->Draw();
    }
}

void AmmoManager::AddAmmo()
{
    int newIndex = static_cast<int>(ammo_.size());
    auto newAmmo = std::make_unique<Ammo>(engine_, newIndex, managerGroupName_);
    newAmmo->SetManager(this->GetManager());
    newAmmo->Initialize();
    newAmmo->GetModel()->ShareMaterialsFrom(sharedModel_.get());

    ammo_.push_back(std::move(newAmmo));

    ammoCount_ = static_cast<int>(ammo_.size());
}

void AmmoManager::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::Begin("アモマネージャー");
    binder_->Draw("AmmoCount", "アモの数");

    if (ImGui::Button("新しいアモを追加"))
    {
        AddAmmo();
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

    if (!ammo_.empty())
    {
        binder_->DrawModel("SharedAmmoModel", "アモインスペクター");
        binder_->DrawModel("SharedBubbleModel", "バブルインスペクター");

        for (auto& ammo : ammo_)
        {
            ammo->GetModel()->ShareMaterialsFrom(sharedModel_.get());
            if (ammo->GetBubbleModel())
            {
                ammo->GetBubbleModel()->ShareMaterialsFrom(sharedBubbleModel_.get());
            }
        }
    }
    ImGui::Separator();

    for (auto& ammo : ammo_)
    {
        ammo->DebugDraw();
    }

    ImGui::End();
#endif
}