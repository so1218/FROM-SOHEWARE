#include "pch.h"
#include "EnvironmentProp.h"
#include "GameDefine.h"
#include "CollisionConfig.h"
#include "Player.h"

using namespace FS;

EnvironmentProp::EnvironmentProp(FS::Engine* engine, int id, const std::string& parentGroupName)
    : engine_(engine), id_(id), parentGroupName_(parentGroupName)
{
    model_ = std::make_unique<FS::Model>(engine_, "cube");
}

EnvironmentProp::~EnvironmentProp()
{
}

void EnvironmentProp::ReleaseEmitters()
{
    for (FS::ParticleEmitter** emitter : { &activeEmitter_, &activeEmitter2_ })
    {
        if (*emitter)
        {
            (*emitter)->Destroy();
            *emitter = nullptr;
        }
    }
}

void EnvironmentProp::Initialize()
{
    const std::string childGroupName = "Prop_" + std::to_string(id_);
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, parentGroupName_, childGroupName);

    // 元のモデルがあれば、形とマテリアルだけ借りる
    if (masterModel_ != nullptr)
    {
        modelName_ = masterModel_->GetName();
        model_ = std::make_unique<FS::Model>(engine_, masterModel_->GetName());
        model_->ShareModelDataFrom(masterModel_);
        model_->ShareMaterialsFrom(masterModel_);
    }

    SetupProperties();
}

void EnvironmentProp::SetupProperties()
{
    // 作り直す前に、前のエミッターを消す
    if (activeEmitter_)
    {
        activeEmitter_->Destroy();
        activeEmitter_ = nullptr;
    }
    if (activeEmitter2_)
    {
        activeEmitter2_->Destroy();
        activeEmitter2_ = nullptr;
    }

    binder_->Clear();

    auto* gv = FS::GlobalVariables::GetInstance();
    const auto& groupPath = binder_->GetGroupPath();

    // 名前を付けていれば読み込む
    std::string loadedCustomName = gv->GetStringValue(groupPath, "CustomName");
    if (!loadedCustomName.empty())
    {
        propCustomName_ = loadedCustomName;
    }
    else
    {
        gv->SetValue(groupPath, "CustomName", propCustomName_);
    }

    // 保存する値の登録
    binder_->Bind("Position", &model_->GetTransform().translation_, { 0.0f, 0.0f, 0.0f });
    binder_->BindRotation("Rotation", &model_->GetTransform().rotation_, 0.01f);
    binder_->Bind("Scale", &model_->GetTransform().scale_, { 1.0f, 1.0f, 1.0f });
    binder_->BindColor("BaseColor", &baseColor_, { 1.0f, 1.0f, 1.0f, 1.0f });

    binder_->Bind("Behavior", &propBehavior_, 0);
    binder_->Bind("HasCollider", &hasCollider_, false);

    binder_->Bind("ColliderType", &colliderType_, 0);
    binder_->Bind("ColliderRadius", &colliderRadius_, 1.0f);
    binder_->Bind("ColliderSize", &colliderSize_, { 1.0f, 1.0f, 1.0f });
    binder_->Bind("ColliderOffset", &colliderOffset_, { 0.0f, 0.0f, 0.0f });

    binder_->Bind("HasLight", &hasLight_, false);
    binder_->BindColor("LightColor", &lightColor_, { 1.0f, 0.5f, 0.0f, 1.0f });
    binder_->Bind("LightIntensity", &lightIntensity_, 5.0f);
    binder_->Bind("LightRadius", &lightRadius_, 10.0f);
    binder_->Bind("LightVolumetricScatteringIntensity", &lightVolumetricScatteringIntensity_, 1.0f);

    // パーティクル 1 つ目
    binder_->Bind("HasParticle", &hasParticle_, false);
    binder_->Bind("IsParticleFollowing", &isParticleFollowing_, true);
    std::string loadedParticle = gv->GetStringValue(groupPath, "ParticleName");
    if (!loadedParticle.empty())
    {
        particleName_ = loadedParticle;
    }
    else
    {
        gv->SetValue(groupPath, "ParticleName", particleName_);
    }

    // パーティクル 2 つ目
    binder_->Bind("HasParticle2", &hasParticle2_, false);
    binder_->Bind("IsParticleFollowing2", &isParticleFollowing2_, true);
    std::string loadedParticle2 = gv->GetStringValue(groupPath, "ParticleName2");
    if (!loadedParticle2.empty())
    {
        particleName2_ = loadedParticle2;
    }
    else
    {
        gv->SetValue(groupPath, "ParticleName2", particleName2_);
    }

    ApplySettings();
}

void EnvironmentProp::ApplySettings()
{
    // コライダーの生成・破棄。形を変えたときは作り直す
    const FS::CollisionShapeType shape = (GetColliderType() == PropColliderType::Sphere) ? FS::CollisionShapeType::Sphere : FS::CollisionShapeType::Box;
    if (collider_ && (!hasCollider_ || collider_->GetType() != shape))
    {
        RemoveCollider(collider_);
        collider_ = nullptr;
    }
    if (hasCollider_ && !collider_)
    {
        collider_ = (shape == FS::CollisionShapeType::Sphere) ? AddSphereCollider(colliderRadius_) : AddBoxCollider(colliderSize_);
        collider_->SetCollisionLayer(CollisionLayer::Prop);
        collider_->SetCollisionMask(CollisionLayer::Player);
    }

    // パーティクルも設定に合わせる
    UpdateParticleEmitter(hasParticle_, particleName_, isParticleFollowing_, activeEmitter_);
    UpdateParticleEmitter(hasParticle2_, particleName2_, isParticleFollowing2_, activeEmitter2_);
}

void EnvironmentProp::ResetState()
{
    SetActive(true);
    // 消えたときに外したパーティクルは、ApplySettings で作り直される
    ApplySettings();
}

void EnvironmentProp::UpdateParticleEmitter(
    bool hasParticle,
    const std::string& particleName,
    bool isFollowing,
    FS::ParticleEmitter*& outEmitter)
{
    if (hasParticle)
    {
        if (!outEmitter)
        {
            outEmitter = engine_->GetParticleSystem()->SpawnEmitter(particleName);
            if (isFollowing)
            {
                outEmitter->SetTargetToFollow(&model_->GetTransform());
            }
            else
            {
                outEmitter->SetPosition(model_->GetTransform().translation_);
            }
        }
    }
    else {
        if (outEmitter) {
            outEmitter->Destroy();
            outEmitter = nullptr;
        }
    }
}

void EnvironmentProp::Update()
{
    if (!IsActive()) return;

    // 調整中に変えた色をすぐ反映する
    model_->SetBaseColor(baseColor_);

    // 調整中に変えた当たり判定の大きさをすぐ反映する
    if (hasCollider_ && collider_)
    {
        if (collider_->GetType() == FS::CollisionShapeType::Sphere)
        {
            collider_->SetRadius(colliderRadius_);
        }
        else
        {
            collider_->SetSize(colliderSize_);
        }
        collider_->SetCenterOffset(colliderOffset_);
    }

    // 光る設定なら、毎フレーム点光源を出す
    if (hasLight_)
    {
        engine_->GetLightManager()->SubmitPointLight(
            GetTransform().translation_,
            lightColor_,
            lightIntensity_,
            lightRadius_,
            lightVolumetricScatteringIntensity_
        );
    }

    // GameObject の位置をモデルに合わせる
    SetTransform(model_->GetTransform());
}

void EnvironmentProp::Draw()
{
    if (!IsActive()) return;

    if (model_)
    {
        model_->Draw();
    }
}

void EnvironmentProp::DebugGui()
{
    ImGui::PushID(id_);

    // -------------------------------------------------------------
    // 位置と見た目
    // -------------------------------------------------------------
    if (ImGui::TreeNodeEx("トランスフォーム & 外観", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Position", "位置");
        binder_->Draw("Rotation", "回転");
        binder_->Draw("Scale", "スケール");
        binder_->Draw("BaseColor", "基本色");
        ImGui::TreePop();
    }

    ImGui::Spacing();

    // -------------------------------------------------------------
    // 振る舞い
    // -------------------------------------------------------------
    if (ImGui::TreeNodeEx("振る舞い", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const char* behaviorNames[] = { "なし", "回収時消滅", "押し戻し" };
        int currentBehavior = propBehavior_;

        if (ImGui::Combo("挙動タイプ", &currentBehavior, behaviorNames, IM_ARRAYSIZE(behaviorNames)))
        {
            propBehavior_ = currentBehavior;
            FS::GlobalVariables::GetInstance()->SetValue(binder_->GetGroupPath(), "Behavior", propBehavior_);
        }
        ImGui::TreePop();
    }

    ImGui::Spacing();

    // -------------------------------------------------------------
    // 当たり判定
    // -------------------------------------------------------------
    if (ImGui::TreeNodeEx("コライダー", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (binder_->Draw("HasCollider", "コライダーを有効化"))
        {
            ApplySettings();
        }

        if (hasCollider_)
        {
            ImGui::Indent();
            if (ImGui::Combo("形状タイプ", &colliderType_, "Sphere\0Box\0"))
            {
                FS::GlobalVariables::GetInstance()->SetValue(binder_->GetGroupPath(), "ColliderType", colliderType_);
                ApplySettings();
            }

            if (GetColliderType() == PropColliderType::Sphere)
            {
                if (binder_->Draw("ColliderRadius", "球半径")) ApplySettings();
            }
            else
            {
                if (binder_->Draw("ColliderSize", "ボックスの大きさ")) ApplySettings();
            }

            if (binder_->Draw("ColliderOffset", "中心オフセット")) ApplySettings();
            ImGui::Unindent();
        }
        ImGui::TreePop();
    }

    ImGui::Spacing();

    // -------------------------------------------------------------
    // 点光源
    // -------------------------------------------------------------
    if (ImGui::TreeNodeEx("ポイントライト", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (binder_->Draw("HasLight", "ポイントライトを有効化"))
        {
            ApplySettings();
        }

        if (hasLight_)
        {
            ImGui::Indent();
            binder_->Draw("LightColor", "ライトカラー");
            binder_->Draw("LightIntensity", "輝度");
            binder_->Draw("LightRadius", "照射半径");
            binder_->Draw("LightVolumetricScatteringIntensity", "フォグ散乱強度");
            ImGui::Unindent();
        }
        ImGui::TreePop();
    }

    ImGui::Spacing();

    // -------------------------------------------------------------
    // パーティクル
    // -------------------------------------------------------------
    if (ImGui::TreeNodeEx("パーティクルエミッター", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 選べるパーティクルの一覧
        std::vector<std::string> presetNames = engine_->GetParticleSystem()->GetPresetNames();

        auto drawParticleUI = [&](
            const char* sectionTitle,
            bool& hasPart,
            bool& isFollow,
            std::string& partName,
            FS::ParticleEmitter*& activeEmit,
            const char* bindHas,
            const char* bindFollow,
            const char* jsonKey,
            const char* idSuffix)
            {
                if (binder_->Draw(bindHas, sectionTitle))
                {
                    ApplySettings();
                }

                if (hasPart)
                {
                    ImGui::Indent();

                    // プロップに付いていくか
                    if (binder_->Draw(bindFollow, "トランスフォーム追従"))
                    {
                        if (activeEmit)
                        {
                            activeEmit->Destroy();
                            activeEmit = nullptr;
                        }
                        ApplySettings();
                    }

                    // どのパーティクルを出すか
                    std::string comboLabel = "アセット名##" + std::string(idSuffix);
                    ImGui::SetNextItemWidth(200.0f);

                    if (ImGui::BeginCombo(comboLabel.c_str(), partName.c_str()))
                    {
                        for (const auto& preset : presetNames)
                        {
                            bool isSelected = (partName == preset);

                            if (ImGui::Selectable(preset.c_str(), isSelected))
                            {
                                // 選び直したら作り直す
                                partName = preset;
                                FS::GlobalVariables::GetInstance()->SetValue(binder_->GetGroupPath(), jsonKey, partName);

                                // 古いエミッターを消して、新しいものを作る
                                if (activeEmit)
                                {
                                    activeEmit->Destroy();
                                    activeEmit = nullptr;
                                }
                                ApplySettings();
                            }

                            if (isSelected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Unindent();
                }
            };

        drawParticleUI("スロット 1", hasParticle_, isParticleFollowing_, particleName_, activeEmitter_, "HasParticle", "IsParticleFollowing", "ParticleName", "P1");
        ImGui::Separator();
        drawParticleUI("スロット 2", hasParticle2_, isParticleFollowing2_, particleName2_, activeEmitter2_, "HasParticle2", "IsParticleFollowing2", "ParticleName2", "P2");

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void EnvironmentProp::OnCollisionEnter(FS::Collider* mine, FS::Collider* other)
{
    if (!IsActive()) return;

    OnCollisionStay(mine, other);
}

void EnvironmentProp::OnCollisionStay(FS::Collider* mine, FS::Collider* other)
{
    if (!IsActive()) return;

    FS::GameObject* hitObj = other->GetOwner();
    if (!hitObj || !hitObj->CompareTag(ObjectTag::Player)) return;

    // 拾ったら消える
    if (GetBehavior() == PropBehavior::Disappear)
    {
        SetActive(false);

        // 付いていたパーティクルも消す
        if (activeEmitter_)
        {
            activeEmitter_->Destroy();
            activeEmitter_ = nullptr;
        }

        if (activeEmitter2_)
        {
            activeEmitter2_->Destroy();
            activeEmitter2_ = nullptr;
        }
    }
    // 押し戻す
    else if (GetBehavior() == PropBehavior::PushBack)
    {
        FS::Vector3 pushVector;
        if (mine->CalculatePushBackVector(other, pushVector))
        {
            hitObj->GetTransform().translation_ += pushVector;

            // 上に押し戻されたら、プロップの上に立っている
            if (pushVector.y > 0.0f)
            {
                Player* player = static_cast<Player*>(hitObj);
                player->SetVelocityY(0.0f);       
                player->SetGroundedOnObject(true); 
            }
        }
    }
}

void EnvironmentProp::OnCollisionExit(FS::Collider* mine, FS::Collider* other)
{

}

void EnvironmentProp::ReassignID(int newID)
{
    id_ = newID;
    const std::string childGroupName = "Prop_" + std::to_string(id_);

    // 番号が変わったので、PropertyBinder を新しい保存先で作り直す
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, parentGroupName_, childGroupName);

    SetupProperties();

    // 新しい保存先にすぐ書いておく
    FS::GlobalVariables::GetInstance()->SaveFile(binder_->GetGroupPath());
}