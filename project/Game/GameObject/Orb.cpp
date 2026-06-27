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

void Orb::Initialize()
{

    collider_->RegisterToManager();

    // 自分にオーブタグを設定
    SetTag(ObjectTag::Orb);

    binder_->BindModel("orbModel", model_.get());
}

void Orb::Update()
{
    //if (model_)
    //{
    //    model_->SetTransform(GetTransform());
    //}
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