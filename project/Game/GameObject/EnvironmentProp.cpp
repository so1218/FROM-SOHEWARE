#include "pch.h"
#include "EnvironmentProp.h"
#include "GameDefine.h"
#include "CollisionConfig.h"
#include "Player.h"

using namespace FE;

EnvironmentProp::EnvironmentProp(FE::Engine* engine, int id, const std::string& parentGroupName)
    : engine_(engine), id_(id), parentGroupName_(parentGroupName)
{
    model_ = std::make_unique<FE::Model>(engine_, "cube");
}

EnvironmentProp::~EnvironmentProp()
{
    // アロケート済みのポイントライトを確実に返却
    if (pointLightIndex_ != -1)
    {
        engine_->GetLightManager()->ReturnPointLight(pointLightIndex_);
        pointLightIndex_ = -1;
    }

    // アクティブなエミッターの安全な解放
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

void EnvironmentProp::Initialize()
{
    const std::string childGroupName = "Prop_" + std::to_string(id_);
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, parentGroupName_, childGroupName);

    // マスターモデルが存在する場合、メッシュおよびマテリアルデータのみを共有
    if (masterModel_ != nullptr)
    {
        modelName_ = masterModel_->GetName();
        model_ = std::make_unique<FE::Model>(engine_, masterModel_->GetName());
        model_->ShareModelDataFrom(masterModel_);
        model_->ShareMaterialsFrom(masterModel_);
    }

    SetupProperties();
}

void EnvironmentProp::SetupProperties()
{
    // 既存のエミッターをリセット
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

    auto* gv = FE::GlobalVariables::GetInstance();
    const auto& groupPath = binder_->GetGroupPath();

    // カスタム名の読み込みおよび初期設定
    std::string loadedCustomName = gv->GetStringValue(groupPath, "CustomName");
    if (!loadedCustomName.empty())
    {
        propCustomName_ = loadedCustomName;
    }
    else
    {
        gv->SetValue(groupPath, "CustomName", propCustomName_);
    }

    // PropertyBinder へのパラメータ登録
    binder_->Bind("Position", &model_->GetTransform().translation_, { 0.0f, 0.0f, 0.0f });
    binder_->BindRotation("Rotation", &model_->GetTransform().rotation_, &model_->GetTransform().rotationQuaternion_, 0.01f);
    binder_->Bind("Scale", &model_->GetTransform().scale_, { 1.0f, 1.0f, 1.0f });
    binder_->BindColor("BaseColor", &baseColor_, { 1.0f, 1.0f, 1.0f, 1.0f });

    binder_->Bind("Behavior", &propBehavior_, 0);
    binder_->Bind("HasCollider", &hasCollider_, false);

    binder_->Bind("ColliderType", &colliderType_, 0);
    binder_->Bind("ColliderRadius", &colliderRadius_, 1.0f);
    binder_->Bind("ColliderSize", &colliderSize_, { 0.5f, 0.5f, 0.5f });
    binder_->Bind("ColliderOffset", &colliderOffset_, { 0.0f, 0.0f, 0.0f });

    binder_->Bind("HasLight", &hasLight_, false);
    binder_->BindColor("LightColor", &lightColor_, { 1.0f, 0.5f, 0.0f, 1.0f });
    binder_->Bind("LightIntensity", &lightIntensity_, 5.0f);
    binder_->Bind("LightRadius", &lightRadius_, 10.0f);
    binder_->Bind("LightVolumetricScatteringIntensity", &lightVolumetricScatteringIntensity_, 1.0f);

    // パーティクル設定 1
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

    // パーティクル設定 2
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
    // コライダーの動的生成・破棄
    if (hasCollider_) 
    {
        if (!collider_) 
        {
            collider_ = std::make_unique<FE::Collider>(this);
            collider_->SetCollisionAttribute(kCollisionAttributeProp);
            collider_->SetCollisionMask(kCollisionAttributePlayer);
            collider_->RegisterToManager();
        }
    }
    else {
        collider_.reset();
    }

    // ポイントライトの動的更新・返却
    if (hasLight_) {
        if (pointLightIndex_ == -1) 
        {
            pointLightIndex_ = engine_->GetLightManager()->RequestPointLight();
        }

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
    else 
    {
        if (pointLightIndex_ != -1) 
        {
            engine_->GetLightManager()->ReturnPointLight(pointLightIndex_);
            pointLightIndex_ = -1;
        }
    }

    // パーティクルエミッターの動的割り当て
    UpdateParticleEmitter(hasParticle_, particleName_, isParticleFollowing_, activeEmitter_);
    UpdateParticleEmitter(hasParticle2_, particleName2_, isParticleFollowing2_, activeEmitter2_);
}

void EnvironmentProp::UpdateParticleEmitter(
    bool hasParticle,
    const std::string& particleName,
    bool isFollowing,
    FE::ParticleEmitter*& outEmitter)
{
    if (hasParticle)
    {
        if (!outEmitter)
        {
            auto emitter = engine_->GetParticleSystem()->CreateEmitter(particleName);
            if (emitter) 
            {
                if (isFollowing)
                {
                    emitter->SetTargetToFollow(&model_->GetTransform());
                }
                else 
                {
                    emitter->SetPosition(model_->GetTransform().translation_);
                }
                outEmitter = emitter.get();
                engine_->GetParticleSystem()->AddEmitter(std::move(emitter));
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

    // マテリアルカラーの適用
    model_->SetBaseColor(baseColor_);

    // 物理・判定用コライダーの形状パラメータ同期
    if (hasCollider_ && collider_)
    {
        collider_->SetType(static_cast<FE::CollisionShapeType>(colliderType_));
        collider_->SetRadius(colliderRadius_);
        collider_->SetSize(colliderSize_);
        collider_->SetCenterOffset(colliderOffset_);
    }

    // 動的ポイントライトのワールド座標および描画パラメータ同期
    if (hasLight_ && pointLightIndex_ != -1)
    {
        const FE::Vector3& currentPos = model_->GetTransform().translation_;
        engine_->GetLightManager()->UpdatePointLightPosition(pointLightIndex_, currentPos);
        engine_->GetLightManager()->UpdatePointLightProperties(
            pointLightIndex_, lightColor_, lightIntensity_, lightRadius_, lightVolumetricScatteringIntensity_
        );
    }

    // ゲームオブジェクト基底のTransformとモデルTransformの同期
    SetTransform(model_->GetTransform());
}

void EnvironmentProp::Draw()
{
    if (!IsActive()) return;

    if (model_)
    {
        model_->Draw();
    }

    if (hasCollider_ && collider_)
    {
        collider_->DrawCollider();
    }
}

void EnvironmentProp::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::PushID(id_);

    // -------------------------------------------------------------
    // トランスフォーム & 外観設定
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
        int currentBehavior = static_cast<int>(propBehavior_);

        if (ImGui::Combo("挙動タイプ", &currentBehavior, behaviorNames, IM_ARRAYSIZE(behaviorNames)))
        {
            propBehavior_ = currentBehavior;
            FE::GlobalVariables::GetInstance()->SetValue(binder_->GetGroupPath(), "Behavior", propBehavior_);
        }
        ImGui::TreePop();
    }

    ImGui::Spacing();

    // -------------------------------------------------------------
    // コライダー設定
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
                FE::GlobalVariables::GetInstance()->SetValue(binder_->GetGroupPath(), "ColliderType", colliderType_);
                ApplySettings();
            }

            if (colliderType_ == 0)
            {
                if (binder_->Draw("ColliderRadius", "球半径")) ApplySettings();
            }
            else
            {
                if (binder_->Draw("ColliderSize", "ボックスサイズ")) ApplySettings();
            }

            if (binder_->Draw("ColliderOffset", "中心オフセット")) ApplySettings();
            ImGui::Unindent();
        }
        ImGui::TreePop();
    }

    ImGui::Spacing();

    // -------------------------------------------------------------
    // ライト設定
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
    // パーティクル設定
    // -------------------------------------------------------------
    if (ImGui::TreeNodeEx("パーティクルエミッター", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 登録済みパーティクルプリセット一覧を取得
        std::vector<std::string> presetNames = engine_->GetParticleSystem()->GetPresetNames();

        auto drawParticleUI = [&](
            const char* sectionTitle,
            bool& hasPart,
            bool& isFollow,
            std::string& partName,
            FE::ParticleEmitter*& activeEmit,
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

                    // トランスフォーム追従の切り替え
                    if (binder_->Draw(bindFollow, "トランスフォーム追従"))
                    {
                        if (activeEmit)
                        {
                            activeEmit->Destroy();
                            activeEmit = nullptr;
                        }
                        ApplySettings();
                    }

                    // パーティクル選択 
                    std::string comboLabel = "アセット名##" + std::string(idSuffix);
                    ImGui::SetNextItemWidth(200.0f);

                    if (ImGui::BeginCombo(comboLabel.c_str(), partName.c_str()))
                    {
                        for (const auto& preset : presetNames)
                        {
                            bool isSelected = (partName == preset);

                            if (ImGui::Selectable(preset.c_str(), isSelected))
                            {
                                // パーティクル名が変更されたら適用
                                partName = preset;
                                FE::GlobalVariables::GetInstance()->SetValue(binder_->GetGroupPath(), jsonKey, partName);

                                // 古いエミッターを破棄して新しいエミッターをリロード
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
#endif
}

void EnvironmentProp::OnCollisionEnter(FE::Collider* mine, FE::Collider* other)
{
    if (!IsActive()) return;

    OnCollisionStay(mine, other);
}

void EnvironmentProp::OnCollisionStay(FE::Collider* mine, FE::Collider* other)
{
    if (!IsActive()) return;

    FE::GameObject* hitObj = other->GetOwner();
    if (!hitObj || !hitObj->CompareTag(ObjectTag::Player)) return;

    // 回収時消滅挙動
    if (propBehavior_ == static_cast<int>(PropBehavior::Disappear))
    {
        SetActive(false);

        // 管理システムへライトインデックスを返却
        if (pointLightIndex_ != -1)
        {
            engine_->GetLightManager()->ReturnPointLight(pointLightIndex_);
            pointLightIndex_ = -1;
        }

        // アタッチされているエミッターの解放
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
    // 押し戻し挙動
    else if (propBehavior_ == static_cast<int>(PropBehavior::PushBack))
    {
        FE::Vector3 pushVector;
        if (mine->CalculatePushBackVector(other, pushVector))
        {
            hitObj->GetTransform().translation_ += pushVector;

            // 上方向に押し戻された場合
            if (pushVector.y > 0.0f && hitObj->CompareTag(ObjectTag::Player))
            {
                Player* player = static_cast<Player*>(hitObj);
                player->SetVelocityY(0.0f);       
                player->SetGroundedOnObject(true); 
            }
        }
    }
}

void EnvironmentProp::OnCollisionExit(FE::Collider* mine, FE::Collider* other)
{

}

void EnvironmentProp::ReassignID(int newID)
{
    id_ = newID;
    const std::string childGroupName = "Prop_" + std::to_string(id_);

    // ID変更に伴い、データバインダーおよび保存先パラメータの再構成
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, parentGroupName_, childGroupName);

    SetupProperties();

    // 更新されたグループパスで設定データを即座に永続化
    FE::GlobalVariables::GetInstance()->SaveFile(binder_->GetGroupPath());
}