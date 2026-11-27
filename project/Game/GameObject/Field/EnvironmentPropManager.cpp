#include "pch.h"
#include "EnvironmentPropManager.h"
#include "GlobalVariables.h"
#include "ImGuiManager.h"

EnvironmentPropManager::EnvironmentPropManager(FS::Engine* engine, const std::string& groupName)
    : engine_(engine), managerGroupName_(groupName)
{}

void EnvironmentPropManager::Initialize()
{
    // シーンに入り直したときは前回のプロップが残っているので、作り直す前に消す
    groups_.clear();
    selectedProp_ = nullptr;
    selectedPropIndex_ = -1;

    binder_ = std::make_unique<FS::PropertyBinder>(engine_, managerGroupName_);
    binder_->Bind("PropCount", &propCount_, 0);

    auto* gv = FS::GlobalVariables::GetInstance();

    // 保存してあるプロップを作り直す
    for (int i = 0; i < propCount_; ++i)
    {
        const std::string childGroupName = "Prop_" + std::to_string(i);

        // どのプレハブか
        std::string prefabName = gv->GetStringValue({ managerGroupName_, childGroupName }, "PrefabName");
        std::string fallbackModel = prefabName;

        if (prefabName.empty())
        {
            prefabName = gv->GetStringValue({ managerGroupName_, childGroupName }, "ModelName");
            if (prefabName.empty()) prefabName = "cube";
            fallbackModel = prefabName;
        }

        CreateGroup(prefabName, fallbackModel);

        // プロップを作り、元のモデルを渡す
        auto prop = std::make_unique<EnvironmentProp>(engine_, i, managerGroupName_);
        prop->SetPrefabName(prefabName);
        prop->SetMasterModel(groups_[prefabName].masterModel.get());
        prop->AttachToManager(GetManager(), 0); 
        prop->Initialize();

        groups_[prefabName].instances.push_back(std::move(prop));
    }
}

void EnvironmentPropManager::CreateGroup(const std::string& prefabName, const std::string& fallbackModelName)
{
    if (groups_.find(prefabName) != groups_.end()) return;

    auto& newGroup = groups_[prefabName];
    newGroup.prefabName = prefabName;

    const std::string masterGroupName = "Master_" + prefabName;
    auto* gv = FS::GlobalVariables::GetInstance();

    // このグループで使うモデル名を読む
    std::string loadedModel = gv->GetStringValue({ managerGroupName_, masterGroupName }, "ModelName");
    if (!loadedModel.empty())
    {
        newGroup.modelName = loadedModel;
    }
    else
    {
        newGroup.modelName = fallbackModelName;
        gv->SetValue({ managerGroupName_, masterGroupName }, "ModelName", newGroup.modelName);
    }

    newGroup.masterModel = std::make_unique<FS::Model>(engine_, newGroup.modelName);
    newGroup.binder = std::make_unique<FS::PropertyBinder>(engine_, managerGroupName_, masterGroupName);

    // ImGui の描画中にモデルを作り直すと表示が崩れるので、ここでは覚えておくだけにする
    newGroup.binder->BindModelName("ModelName", &newGroup.modelName, newGroup.modelName,
        [this, prefabName](const std::string& newName) {
            this->pendingModelChangePrefab_ = prefabName;
            this->pendingModelChangeNewName_ = newName;
        });

    newGroup.binder->BindModel("MasterModel", newGroup.masterModel.get());
}

void EnvironmentPropManager::Update()
{
    // ImGui で選ばれたモデルへの差し替えは、フレームの頭で行う
    ExecutePrefabModelChange();

    for (auto& [prefabName, group] : groups_)
    {
        for (auto& prop : group.instances)
        {
            if (prop->IsActive())
            {
                prop->Update();
            }
        }
    }
}

void EnvironmentPropManager::ResetProps()
{
    for (auto& [prefabName, group] : groups_)
    {
        for (auto& prop : group.instances)
        {
            prop->ResetState();
        }
    }
}

void EnvironmentPropManager::Draw()
{
    for (auto& [prefabName, group] : groups_)
    {
        for (auto& prop : group.instances)
        {
            if (prop->IsActive())
            {
                prop->Draw();
            }
        }
    }
}

void EnvironmentPropManager::AddPropToGroup(const std::string& prefabName)
{
    const int newIndex = propCount_;

    CreateGroup(prefabName, "cube");

    auto newProp = std::make_unique<EnvironmentProp>(engine_, newIndex, managerGroupName_);
    newProp->SetPrefabName(prefabName);
    newProp->SetMasterModel(groups_[prefabName].masterModel.get());
    newProp->AttachToManager(GetManager(), 0); 
    newProp->Initialize();

    // 保存データにも足しておく
    FS::GlobalVariables::GetInstance()->SetValue(
        { managerGroupName_, "Prop_" + std::to_string(newIndex) }, "PrefabName", prefabName
    );

    groups_[prefabName].instances.push_back(std::move(newProp));

    propCount_++;
    FS::GlobalVariables::GetInstance()->SetValue({ managerGroupName_ }, "PropCount", propCount_);
}

void EnvironmentPropManager::RemoveEnvironmentProp(int targetId)
{
    // 番号を詰めると最後の番号が空くので、その保存データを消す
    const int lastIndex = propCount_ - 1;
    const std::string lastGroupName = "Prop_" + std::to_string(lastIndex);
    FS::GlobalVariables::GetInstance()->ClearGroup({ managerGroupName_, lastGroupName });

    // 指定した番号のプロップを消す
    for (auto& [prefabName, group] : groups_)
    {
        auto it = std::remove_if(group.instances.begin(), group.instances.end(),
            [targetId](const std::unique_ptr<EnvironmentProp>& p) { return p->GetID() == targetId; });

        if (it != group.instances.end())
        {
            for (auto removed = it; removed != group.instances.end(); ++removed)
            {
                (*removed)->ReleaseEmitters();
            }
            group.instances.erase(it, group.instances.end());
            break;
        }
    }

    // 後ろのプロップの番号を 1 つずつ詰め、保存データも合わせる
    for (auto& [prefabName, group] : groups_)
    {
        for (auto& prop : group.instances)
        {
            if (prop->GetID() > targetId)
            {
                prop->ReassignID(prop->GetID() - 1);

                FS::GlobalVariables::GetInstance()->SetValue(
                    { managerGroupName_, "Prop_" + std::to_string(prop->GetID()) },
                    "PrefabName",
                    prefabName
                );
            }
        }
    }

    propCount_--;
    FS::GlobalVariables::GetInstance()->SetValue({ managerGroupName_ }, "PropCount", propCount_);

    // 選んでいたプロップを消したら、選択を外す
    if (selectedProp_ != nullptr && selectedProp_->GetID() == targetId)
    {
        selectedProp_ = nullptr;
    }
}

void EnvironmentPropManager::ExecutePrefabModelChange()
{
    if (pendingModelChangePrefab_.empty()) return;

    auto it = groups_.find(pendingModelChangePrefab_);
    if (it != groups_.end())
    {
        auto& group = it->second;
        group.modelName = pendingModelChangeNewName_;

        // 元のモデルを作り直し、同じグループの全プロップにもう一度渡す
        group.masterModel = std::make_unique<FS::Model>(engine_, group.modelName);
        for (auto& prop : group.instances)
        {
            prop->ChangeMasterModel(group.masterModel.get());
        }
    }

    pendingModelChangePrefab_.clear();
    pendingModelChangeNewName_.clear();
}

void EnvironmentPropManager::DebugGui()
{
    ImGui::Begin("環境オブジェクトマネージャー");

    int deleteRequestID = -1;

    // プレハブのグループごとに表示
    for (auto& [prefabName, group] : groups_)
    {
        ImGui::PushID(prefabName.c_str());

        const std::string groupHeader = prefabName + " (" + std::to_string(group.instances.size()) + " 個)";

        if (ImGui::CollapsingHeader(groupHeader.c_str()))
        {
            ImGui::Indent();

            // グループ共通のマテリアル
            if (ImGui::TreeNode("共通マテリアル設定 (Prefab)"))
            {
                group.binder->Draw("ModelName", "ベース3Dモデル");

                const bool isMaterialChanged = group.binder->DrawModel("MasterModel", "マテリアル");
                if (isMaterialChanged)
                {
                    for (auto& inst : group.instances)
                    {
                        inst->GetModel()->ShareMaterialsFrom(group.masterModel.get());
                    }
                }
                ImGui::TreePop();
            }

            ImGui::Separator();
            ImGui::TextDisabled("配置済みインスタンス一覧");

            // グループの中のプロップ
            for (size_t i = 0; i < group.instances.size(); ++i)
            {
                auto& prop = group.instances[i];
                ImGui::PushID(prop->GetID());

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (selectedProp_ == prop.get()) flags |= ImGuiTreeNodeFlags_Selected;

                const std::string instName = "[" + std::to_string(prop->GetID()) + "] " + prop->GetDisplayName();
                const bool isOpen = ImGui::TreeNodeEx(instName.c_str(), flags);

                // クリックしたら、ギズモで動かせるようにする
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                {
                    selectedProp_ = prop.get();
                }

                // 開いたら、そのプロップの設定を出す
                if (isOpen)
                {
                    ImGui::Spacing();
                    prop->DebugGui();

                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                    if (ImGui::Button("このオブジェクトを削除", ImVec2(-1, 0)))
                    {
                        deleteRequestID = prop->GetID();
                    }
                    ImGui::PopStyleColor();

                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            ImGui::Spacing();
            if (ImGui::Button("+ 新規インスタンスを追加", ImVec2(-1, 0)))
            {
                AddPropToGroup(prefabName);
            }

            ImGui::Unindent();
        }
        ImGui::PopID();
    }

    // 消すボタンが押されたプロップは、一覧を出し終わってから消す
    if (deleteRequestID != -1)
    {
        RemoveEnvironmentProp(deleteRequestID);
    }

    ImGui::Separator();

    // 新しいプレハブを作る
    if (ImGui::TreeNode("新規 Prefab の登録"))
    {
        static char newPrefabName[128] = "MyNewPrefab";
        static char baseModelName[128] = "cube";

        ImGui::InputText("Prefab識別名", newPrefabName, sizeof(newPrefabName));
        ImGui::InputText("初期3Dモデル名", baseModelName, sizeof(baseModelName));

        if (ImGui::Button("Prefabを作成", ImVec2(-1, 0)))
        {
            CreateGroup(newPrefabName, baseModelName);
        }
        ImGui::TreePop();
    }

    ImGui::End();

    // 選んでいるプロップをギズモで動かす
    if (selectedProp_ != nullptr)
    {
        auto& targetTransform = selectedProp_->GetTransformRef();
        const FS::Vector3 oldPos = targetTransform.translation_;
        const FS::Quaternion oldRotation = targetTransform.rotation_;
        const FS::Vector3 oldScale = targetTransform.scale_;

        FS::ImGuiManager::DrawGizmo(targetTransform);

        // 動かしたら保存データにも書く
        if (oldPos != targetTransform.translation_ ||
            std::abs(FS::Quaternion::Dot(oldRotation, targetTransform.rotation_)) < 0.99999f ||
            oldScale != targetTransform.scale_)
        {
            auto* gv = FS::GlobalVariables::GetInstance();
            const std::vector<std::string> groupPath = { managerGroupName_, "Prop_" + std::to_string(selectedProp_->GetID()) };

            gv->SetValue(groupPath, "Position", targetTransform.translation_);
            gv->SetValue(groupPath, "Rotation", targetTransform.GetRotationEuler());
            gv->SetValue(groupPath, "Scale", targetTransform.scale_);
        }
    }

    // ImGui で選ばれたモデルに差し替える
    ExecutePrefabModelChange();
}

void EnvironmentProp::SyncMaterialsToJSON()
{
    auto* gv = FS::GlobalVariables::GetInstance();
    const std::vector<std::string>& groupPath = binder_->GetGroupPath();
    const std::string prefix = "Model_";

    const size_t count = model_->GetMaterialCount();
    for (size_t i = 0; i < count; ++i)
    {
        // マテリアルが複数あるときは、保存のキーに番号を付けて分ける
        const std::string matPrefix = (count == 1) ? prefix : prefix + "Mat" + std::to_string(i) + "_";

        FS::MaterialHandle* handle = model_->GetMaterialHandle(i);
        MaterialData* matData = model_->GetMaterialData(i);

        if (!handle || !matData) continue;

        // テクスチャ
        gv->SetValue(groupPath, matPrefix + "AlbedoMap", handle->textureName);
        gv->SetValue(groupPath, matPrefix + "EnvMapTex", handle->envMapName);
        gv->SetValue(groupPath, matPrefix + "NormalMapTex", handle->normalMapName);
        gv->SetValue(groupPath, matPrefix + "DissolveTex", handle->dissolveMapName);
        gv->SetValue(groupPath, matPrefix + "ToonRampTex", handle->toonRampName);

        // UV の移動・回転・拡大
        gv->SetValue(groupPath, matPrefix + "UVTrans", handle->uvTransformData.translation);
        gv->SetValue(groupPath, matPrefix + "UVRot", handle->uvTransformData.rotation);
        gv->SetValue(groupPath, matPrefix + "UVScale", handle->uvTransformData.scale);

        // 色とライティング
        gv->SetValue(groupPath, matPrefix + "Color", matData->color);
        gv->SetValue(groupPath, matPrefix + "Lighting", matData->enableLighting);
        gv->SetValue(groupPath, matPrefix + "LightMode", matData->lightMode);
        gv->SetValue(groupPath, matPrefix + "DiffuseRef", matData->diffuseReflection);
        gv->SetValue(groupPath, matPrefix + "Shininess", matData->shininess);
        gv->SetValue(groupPath, matPrefix + "EnvMapInt", matData->environmentMapIntensity);

        // PBR と発光
        gv->SetValue(groupPath, matPrefix + "Roughness", matData->roughness);
        gv->SetValue(groupPath, matPrefix + "Metalness", matData->metalness);
        gv->SetValue(groupPath, matPrefix + "SpecColor", matData->specularColor);
        gv->SetValue(groupPath, matPrefix + "Emissive", matData->emissiveIntensity);
        gv->SetValue(groupPath, matPrefix + "AlphaThres", matData->alphaTestThreshold);

        // 影
        gv->SetValue(groupPath, matPrefix + "AddShadow", matData->addShadow);
        gv->SetValue(groupPath, matPrefix + "ShadowBias", matData->shadowBias);
        gv->SetValue(groupPath, matPrefix + "ShadowDens", matData->shadowDensity);
        gv->SetValue(groupPath, matPrefix + "ShadowEnv", matData->shadowEnvStrength);
        gv->SetValue(groupPath, matPrefix + "ShadowSoft", matData->shadowSoftness);

        // リムライト
        gv->SetValue(groupPath, matPrefix + "RimEnable", matData->enableRim);
        gv->SetValue(groupPath, matPrefix + "RimUseDir", matData->rimUseLightDir);
        gv->SetValue(groupPath, matPrefix + "RimPower", matData->rimPower);
        gv->SetValue(groupPath, matPrefix + "RimInten", matData->rimIntensity);
        gv->SetValue(groupPath, matPrefix + "RimColor", matData->rimColor);

        // ディゾルブ
        gv->SetValue(groupPath, matPrefix + "DisEnable", matData->enableDissolve);
        gv->SetValue(groupPath, matPrefix + "DisThres", matData->dissolveThreshold);
        gv->SetValue(groupPath, matPrefix + "EdgeWidth", matData->edgeWidth);
        gv->SetValue(groupPath, matPrefix + "EdgeInten", matData->edgeIntensity);
        gv->SetValue(groupPath, matPrefix + "EdgeColor", matData->edgeColor);

        // 法線マップ
        gv->SetValue(groupPath, matPrefix + "NormEnable", matData->enableNormalMap);
        gv->SetValue(groupPath, matPrefix + "NormTile", matData->normalTiling);
        gv->SetValue(groupPath, matPrefix + "NormInten", matData->normalIntensity);

        // 輪郭線
        gv->SetValue(groupPath, matPrefix + "OutlineEnable", matData->enableOutline);
        gv->SetValue(groupPath, matPrefix + "OutlineWidth", matData->outlineWidth);
        gv->SetValue(groupPath, matPrefix + "OutlineColor", matData->outlineColor);
    }
}


void EnvironmentProp::ChangeMasterModel(FS::Model* newMaster)
{
    masterModel_ = newMaster;

    // 作り直す前に位置を覚えておく
    const auto currentTransform = model_->GetTransform();

    // 新しい元のモデルで、モデルを作り直す
    modelName_ = masterModel_->GetName();
    model_ = std::make_unique<FS::Model>(engine_, modelName_);

    // 形とマテリアルを借りる
    model_->ShareModelDataFrom(masterModel_);
    model_->ShareMaterialsFrom(masterModel_);

    // 覚えておいた位置に戻す
    model_->SetTransform(currentTransform);

    // モデルを作り直したので、PropertyBinder も登録し直す
    SetupProperties();
}