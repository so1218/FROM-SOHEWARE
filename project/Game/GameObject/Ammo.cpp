#include "pch.h"
#include "Ammo.h"
#include "GameDefine.h"
#include "CollisionConfig.h"
#include "TimeManager.h"

using namespace FE;

Ammo::Ammo(Engine* engine, int id, const std::string& parentGroupName)
    : engine_(engine), id_(id)
{
    model_ = std::make_unique<Model>(engine_, "bullet");
    bubbleModel_ = std::make_unique<Model>(engine_, "sphere");
    std::string childGroupName = "Ammo_" + std::to_string(id_);
    binder_ = std::make_unique<PropertyBinder>(engine_, parentGroupName, childGroupName);
    collider_ = std::make_unique<Collider>(this);
}

Ammo::~Ammo()
{
    // アモが破棄される際、LightManagerに返却
    if (pointLightIndex_ != -1)
    {
        engine_->GetLightManager()->ReturnPointLight(pointLightIndex_);
        pointLightIndex_ = -1;
    }
}

void Ammo::Initialize()
{
    collider_->RegisterToManager();
    SetTag(ObjectTag::Ammo);

    // 各アモ個別の設定
    binder_->Bind("Position", &model_->GetTransform().translation_, { 0.0f, 0.0f, 0.0f });

    binder_->Bind("LightIntensity", &lightIntensity_, 5.0f);
    binder_->Bind("LightRadius", &lightRadius_, 10.0f);
    binder_->Bind("LightVolumetricScatteringIntensity", &lightVolumetricScatteringIntensity_, 1.0f);

    // ポイントライトの空きを要求
    pointLightIndex_ = engine_->GetLightManager()->RequestPointLight();

    hitEmitter_ = engine_->GetParticleSystem()->CreateEmitter("ammoHit");
    if (hitEmitter_)
    {
        hitEmitter_->SetTargetToFollow(&model_->GetTransform());
        hitEmitterPtr_ = hitEmitter_.get();
        engine_->GetParticleSystem()->AddEmitter(std::move(hitEmitter_));
    }

    collider_->SetCollisionAttribute(kCollisionAttributeProp);
    collider_->SetCollisionMask(kCollisionAttributePlayer);
}

void Ammo::Update(const Vector3& scale, const FE::Vector3& bubbleScale, const Vector4& lightColor, float tiltAngle, float rotationSpeed)
{
    if (isPicked_) return;

    // スケールの適用
    model_->GetTransform().scale_ = scale;

    // Y軸自転の回転計算
    rotationAngle_ += rotationSpeed * TimeManager::GetInstance()->GetDeltaTime();

    // X軸に傾けた状態を作成
    Quaternion tiltRot = Quaternion::QuaternionFromEuler({ Math::ToRadians(tiltAngle), 0.0f, 0.0f });
    // 傾いた自軸のY軸まわりに回転
    Quaternion spinRot = Quaternion::QuaternionFromEuler({ 0.0f, rotationAngle_, 0.0f });

    // 傾きを適用した後にY軸で回転
    model_->GetTransform().rotationQuaternion_ = spinRot * tiltRot;

    bubbleModel_->GetTransform().translation_ = model_->GetTransform().translation_;
    bubbleModel_->GetTransform().scale_ = bubbleScale;

    // ポイントライトの追従
    if (pointLightIndex_ != -1)
    {
        Vector3 currentPos = model_->GetTransform().translation_;

        engine_->GetLightManager()->UpdatePointLightPosition(pointLightIndex_, currentPos);
        engine_->GetLightManager()->UpdatePointLightProperties(
            pointLightIndex_,
            lightColor, 
            lightIntensity_,
            lightRadius_,
            lightVolumetricScatteringIntensity_
        );
    }

    SetTransform(model_->GetTransform());
}

void Ammo::Draw()
{
    if (isPicked_) return;

    if (model_)
    {
        model_->Draw();
    }
    if (bubbleModel_)
    {
        bubbleModel_->Draw();
    }

    collider_->DrawCollider();
}

void Ammo::DebugDraw()
{
#ifdef ENABLE_IMGUI

    ImGui::PushID(id_);

    std::string headerName = "アモ " + std::to_string(id_);

    if (ImGui::CollapsingHeader(headerName.c_str()))
    {
        ImGui::Indent(); 

        ImGui::Text("基本設定");
        binder_->Draw("Position", "座標");

        ImGui::Separator();

        ImGui::Text("ライト設定");
        binder_->Draw("LightIntensity", "明るさ");
        binder_->Draw("LightRadius", "影響範囲");
        binder_->Draw("LightVolumetricScatteringIntensity", "ボリュームフォグ輝度");

        ImGui::Unindent(); 
        ImGui::Spacing();  
    }

    ImGui::PopID();

#endif
}

void Ammo::OnCollisionStay(Collider* mine, Collider* other)
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

void Ammo::Sleep()
{
    isPicked_ = true;
    SetActive(false);

    if (pointLightIndex_ != -1)
    {
        engine_->GetLightManager()->UpdatePointLightProperties(
            pointLightIndex_, { 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f
        );
    }
}
