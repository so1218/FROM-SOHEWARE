#include "pch.h"
#include "Orb.h"
#include "GameDefine.h"

using namespace FE;

Orb::Orb(Engine* engine, int id) : engine_(engine), id_(id)
{
    model_ = std::make_unique<Model>(engine_, "sphere");

    binder_ = std::make_unique<PropertyBinder>(engine_, "Orb", std::to_string(id_));

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

    // 自分にオーブタグを設定
    SetTag(ObjectTag::Orb);

    binder_->BindModel("orbModel", model_.get());

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
}

void Orb::Update()
{
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
    if (model_)
    {
        model_->Draw();
    }
    collider_->DrawCollider();
}

void Orb::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("オーブ");

    ImGui::PushID(id_);

    std::string label = "オーブ " + std::to_string(id_) + " のインスペクター";

    binder_->DrawModel("orbModel", label);

    ImGui::Separator();
    ImGui::Text("ライト設定");
    binder_->Draw("LightColor", "ライトの色");
    binder_->Draw("LightIntensity", "明るさ");
    binder_->Draw("LightRadius", "影響範囲");
    binder_->Draw("LightVolumetricScatteringIntensity", "ボリュームフォグ輝度");

    ImGui::PopID();

    ImGui::End();
#endif
}

void Orb::OnCollisionStay(Collider* mine, Collider* other)
{
    GameObject* hitObject = other->GetOwner();

    if (hitObject && hitObject->CompareTag(ObjectTag::Player))
    {
        // プレイヤーとぶつかったら自分を消滅
        Destroy();
    }
}