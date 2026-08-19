#include "pch.h"
#include "Orb.h"
#include "GameDefine.h"

using namespace FE;

Orb::Orb(Engine* engine, int id, const std::string& parentGroupName)
    : engine_(engine), id_(id)
{
    model_ = std::make_unique<Model>(engine_, "sphere");
    std::string childGroupName = "Orb_" + std::to_string(id_);
    binder_ = std::make_unique<PropertyBinder>(engine_, parentGroupName, childGroupName);
    collider_ = std::make_unique<Collider>(this);
}

Orb::~Orb()
{
    // オーブが破棄される際、ライトを借りていればLightManagerに返却
    if (pointLightIndex_ != -1)
    {
        engine_->GetLightManager()->ReturnPointLight(pointLightIndex_);
        pointLightIndex_ = -1;
    }
}

void Orb::Initialize()
{
    collider_->RegisterToManager();

    SetTag(ObjectTag::Orb);

    binder_->Bind("Position", &model_->GetTransform().translation_, { 0.0f, 0.0f, 0.0f });
    binder_->Bind("Scale", &model_->GetTransform().scale_, { 1.0f, 1.0f, 1.0f });

    binder_->BindColor("LightColor", &lightColor_, { 0.2f, 0.6f, 1.0f, 1.0f });
    binder_->Bind("LightIntensity", &lightIntensity_, 5.0f);
    binder_->Bind("LightRadius", &lightRadius_, 10.0f);
    binder_->Bind("LightVolumetricScatteringIntensity", &lightVolumetricScatteringIntensity_, 1.0f);

    // ポイントライトの空きを要求
    pointLightIndex_ = engine_->GetLightManager()->RequestPointLight();

    if (pointLightIndex_ != -1)
    {
        engine_->GetLightManager()->UpdatePointLightProperties(
            pointLightIndex_,
            lightColor_,
            lightIntensity_,
            lightRadius_,
            lightVolumetricScatteringIntensity_
        );
    }
    hitEmitter_ = engine_->GetParticleSystem()->CreateEmitter("orbHit");
    hitEmitter_->SetTargetToFollow(&model_->GetTransform());
    hitEmitterPtr_ = hitEmitter_.get();
    engine_->GetParticleSystem()->AddEmitter(std::move(hitEmitter_));
}

void Orb::Update()
{
    if (isPicked_) return;

    // オーブの現在座標にライトを追従
    if (pointLightIndex_ != -1)
    {
        Vector3 currentPos = model_->GetTransform().translation_;

        engine_->GetLightManager()->UpdatePointLightPosition(pointLightIndex_, currentPos);

        engine_->GetLightManager()->UpdatePointLightProperties(
            pointLightIndex_,
            lightColor_,
            lightIntensity_,
            lightRadius_,
            lightVolumetricScatteringIntensity_
        );
    }

    SetTransform(model_->GetTransform());
}

void Orb::Draw()
{
    if (isPicked_) return;

    if (model_)
    {
        model_->Draw();
    }
    collider_->DrawCollider();
}

void Orb::DebugDraw()
{
#ifdef ENABLE_IMGUI

    ImGui::PushID(id_);

    std::string headerName = "オーブ " + std::to_string(id_);

    if (ImGui::CollapsingHeader(headerName.c_str()))
    {
        ImGui::Indent(); 

        ImGui::Text("基本設定");
        binder_->Draw("Position", "座標");
        binder_->Draw("Scale", "スケール");

        ImGui::Separator();

        ImGui::Text("ライト設定");
        binder_->Draw("LightColor", "ライトの色");
        binder_->Draw("LightIntensity", "明るさ");
        binder_->Draw("LightRadius", "影響範囲");
        binder_->Draw("LightVolumetricScatteringIntensity", "ボリュームフォグ輝度");

        ImGui::Unindent(); 
        ImGui::Spacing();  
    }

    ImGui::PopID();

#endif
}

void Orb::OnCollisionStay(Collider* mine, Collider* other)
{
    GameObject* hitObject = other->GetOwner();

    if (hitObject && hitObject->CompareTag(ObjectTag::Player))
    {
        Sleep(); 

        if (hitEmitterPtr_)
        {
            hitEmitterPtr_->Play();
        }
    }
}

void Orb::Sleep()
{
    isPicked_ = true;

    SetActive(false);

    // ライトを見えなくする
    if (pointLightIndex_ != -1)
    {
        engine_->GetLightManager()->UpdatePointLightProperties(
            pointLightIndex_, lightColor_, 0.0f, 0.0f, 0.0f
        );
    }
}
