#include "pch.h"
#include "EnvironmentPropManager.h"
#include "GlobalVariables.h"
#include "ImGuiManager.h"

EnvironmentPropManager::EnvironmentPropManager(FE::Engine* engine, const std::string& groupName)
    : engine_(engine), managerGroupName_(groupName)
{}

void EnvironmentPropManager::Initialize()
{
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, managerGroupName_);
    binder_->Bind("PropCount", &propCount_, 0);

    auto* gv = FE::GlobalVariables::GetInstance();

    // 永続化されたプロップデータの復元
    for (int i = 0; i < propCount_; ++i)
    {
        const std::string childGroupName = "Prop_" + std::to_string(i);

        // プレハブ名の取得
        std::string prefabName = gv->GetStringValue({ managerGroupName_, childGroupName }, "PrefabName");
        std::string fallbackModel = prefabName;

        if (prefabName.empty())
        {
            prefabName = gv->GetStringValue({ managerGroupName_, childGroupName }, "ModelName");
            if (prefabName.empty()) prefabName = "cube";
            fallbackModel = prefabName;
        }

        CreateGroup(prefabName, fallbackModel);

        // プロップの生成とマスターモデルの参照割り当て
        auto prop = std::make_unique<EnvironmentProp>(engine_, i, managerGroupName_);
        prop->SetPrefabName(prefabName);
        prop->SetMasterModel(groups_[prefabName].masterModel.get());
        prop->SetManager(this->GetManager());
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
    auto* gv = FE::GlobalVariables::GetInstance();

    // グループで使用するモデル名の読み込みおよび初期設定
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

    newGroup.masterModel = std::make_unique<FE::Model>(engine_, newGroup.modelName);
    newGroup.binder = std::make_unique<FE::PropertyBinder>(engine_, managerGroupName_, masterGroupName);

    // ImGui描画フレーム内での即時モデル構築による不整合を防ぐため、遅延適用用の変数へ書き込み
    newGroup.binder->BindModelName("ModelName", &newGroup.modelName, newGroup.modelName,
        [this, prefabName](const std::string& newName) {
            this->pendingModelChangePrefab_ = prefabName;
            this->pendingModelChangeNewName_ = newName;
        });

    newGroup.binder->BindModel("MasterModel", newGroup.masterModel.get());
}

void EnvironmentPropManager::Update()
{
    // フレーム先頭でのモデル遅延再構築の適用
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
    newProp->SetManager(this->GetManager());
    newProp->Initialize();

    // 永続化用データストアへ新規インスタンスを記録
    FE::GlobalVariables::GetInstance()->SetValue(
        { managerGroupName_, "Prop_" + std::to_string(newIndex) }, "PrefabName", prefabName
    );

    groups_[prefabName].instances.push_back(std::move(newProp));

    propCount_++;
    FE::GlobalVariables::GetInstance()->SetValue({ managerGroupName_ }, "PropCount", propCount_);
}

void EnvironmentPropManager::RemoveEnvironmentProp(int targetId)
{
    // 末尾ノードの削除と永続化データのクリア
    const int lastIndex = propCount_ - 1;
    const std::string lastGroupName = "Prop_" + std::to_string(lastIndex);
    FE::GlobalVariables::GetInstance()->ClearGroup({ managerGroupName_, lastGroupName });

    // 指定IDインスタンスの削除
    for (auto& [prefabName, group] : groups_)
    {
        auto it = std::remove_if(group.instances.begin(), group.instances.end(),
            [targetId](const std::unique_ptr<EnvironmentProp>& p) { return p->GetID() == targetId; });

        if (it != group.instances.end())
        {
            group.instances.erase(it, group.instances.end());
            break;
        }
    }

    // 後続オブジェクトのID繰り上げおよび設定データの再同期
    for (auto& [prefabName, group] : groups_)
    {
        for (auto& prop : group.instances)
        {
            if (prop->GetID() > targetId)
            {
                prop->ReassignID(prop->GetID() - 1);

                FE::GlobalVariables::GetInstance()->SetValue(
                    { managerGroupName_, "Prop_" + std::to_string(prop->GetID()) },
                    "PrefabName",
                    prefabName
                );
            }
        }
    }

    propCount_--;
    FE::GlobalVariables::GetInstance()->SetValue({ managerGroupName_ }, "PropCount", propCount_);

    // 削除対象が選択中だった場合のポインタクリア
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

        // マスターモデルの生成と参照共有の全インスタンス再同期
        group.masterModel = std::make_unique<FE::Model>(engine_, group.modelName);
        for (auto& prop : group.instances)
        {
            prop->ChangeMasterModel(group.masterModel.get());
        }
    }

    pendingModelChangePrefab_.clear();
    pendingModelChangeNewName_.clear();
}

void EnvironmentPropManager::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::Begin("環境オブジェクトマネージャー");

    int deleteRequestID = -1;

    // プレハブグループごとの描画
    for (auto& [prefabName, group] : groups_)
    {
        ImGui::PushID(prefabName.c_str());

        const std::string groupHeader = prefabName + " (" + std::to_string(group.instances.size()) + " 個)";

        if (ImGui::CollapsingHeader(groupHeader.c_str()))
        {
            ImGui::Indent();

            // 共通マテリアル設定
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

            // 各インスタンスの描画
            for (size_t i = 0; i < group.instances.size(); ++i)
            {
                auto& prop = group.instances[i];
                ImGui::PushID(prop->GetID());

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (selectedProp_ == prop.get()) flags |= ImGuiTreeNodeFlags_Selected;

                const std::string instName = "[" + std::to_string(prop->GetID()) + "] " + prop->GetDisplayName();
                const bool isOpen = ImGui::TreeNodeEx(instName.c_str(), flags);

                // ノードクリック時にギズモ操作対象として選択
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                {
                    selectedProp_ = prop.get();
                }

                // ツリーが開かれたら直接パラメータUIを表示 
                if (isOpen)
                {
                    ImGui::Spacing();
                    prop->DebugDraw();

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

    // 削除リクエストの集約処理
    if (deleteRequestID != -1)
    {
        RemoveEnvironmentProp(deleteRequestID);
    }

    ImGui::Separator();

    // 新規プレハブ構築UI
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

    // 選択オブジェクトへの3Dギズモ描画およびトランスフォーム同期
    if (selectedProp_ != nullptr)
    {
        auto& targetTransform = selectedProp_->GetTransformRef();
        const FE::Vector3 oldPos = targetTransform.translation_;
        const FE::Vector3 oldRot = targetTransform.rotation_;
        const FE::Vector3 oldScale = targetTransform.scale_;

        FE::ImGuiManager::DrawGizmo(targetTransform);

        // ギズモ操作によるトランスフォーム変更を検知してデータストアへ永続化
        if (oldPos != targetTransform.translation_ ||
            oldRot != targetTransform.rotation_ ||
            oldScale != targetTransform.scale_)
        {
            auto* gv = FE::GlobalVariables::GetInstance();
            const std::vector<std::string> groupPath = { managerGroupName_, "Prop_" + std::to_string(selectedProp_->GetID()) };

            gv->SetValue(groupPath, "Position", targetTransform.translation_);
            gv->SetValue(groupPath, "Rotation", targetTransform.rotation_);
            gv->SetValue(groupPath, "Scale", targetTransform.scale_);
        }
    }

    // モデル非同期差し替えの適用
    ExecutePrefabModelChange();
#endif
}

void EnvironmentProp::SyncMaterialsToJSON()
{
    auto* gv = FE::GlobalVariables::GetInstance();
    const std::vector<std::string>& groupPath = binder_->GetGroupPath();
    const std::string prefix = "Model_";

    const size_t count = model_->GetMaterialCount();
    for (size_t i = 0; i < count; ++i)
    {
        // 複数マテリアル保持時はインデックス付きプレフィックスを自動生成
        const std::string matPrefix = (count == 1) ? prefix : prefix + "Mat" + std::to_string(i) + "_";

        FE::MaterialHandle* handle = model_->GetMaterialHandle(i);
        MaterialData* matData = model_->GetMaterialData(i);

        if (!handle || !matData) continue;

        // テクスチャ名
        gv->SetValue(groupPath, matPrefix + "AlbedoMap", handle->textureName);
        gv->SetValue(groupPath, matPrefix + "EnvMapTex", handle->envMapName);
        gv->SetValue(groupPath, matPrefix + "NormalMapTex", handle->normalMapName);
        gv->SetValue(groupPath, matPrefix + "HeightMapTex", handle->heightMapName);
        gv->SetValue(groupPath, matPrefix + "DissolveTex", handle->dissolveMapName);
        gv->SetValue(groupPath, matPrefix + "ToonRampTex", handle->toonRampName);
        gv->SetValue(groupPath, matPrefix + "RippleMap", handle->rippleTextureName);
        gv->SetValue(groupPath, matPrefix + "PuddleNoise", handle->puddleNoiseName);

        // UVトランスフォーム
        gv->SetValue(groupPath, matPrefix + "UVTrans", handle->uvTransformData.translation_);
        gv->SetValue(groupPath, matPrefix + "UVRot", handle->uvTransformData.rotation_);
        gv->SetValue(groupPath, matPrefix + "UVScale", handle->uvTransformData.scale_);

        // トライプランナー設定
        gv->SetValue(groupPath, matPrefix + "UseTriplanar", matData->useTriplanar);
        gv->SetValue(groupPath, matPrefix + "TriScale", matData->triplanarScale);
        gv->SetValue(groupPath, matPrefix + "TriSharpness", matData->triplanarBlendSharpness);

        // 基本色・ライティング設定
        gv->SetValue(groupPath, matPrefix + "Color", matData->color);
        gv->SetValue(groupPath, matPrefix + "Lighting", matData->enableLighting);
        gv->SetValue(groupPath, matPrefix + "LightMode", matData->lightMode);
        gv->SetValue(groupPath, matPrefix + "DiffuseRef", matData->diffuseReflection);
        gv->SetValue(groupPath, matPrefix + "Shininess", matData->shininess);
        gv->SetValue(groupPath, matPrefix + "EnvMapInt", matData->environmentMapIntensity);

        // PBR・エミッシブ設定
        gv->SetValue(groupPath, matPrefix + "Roughness", matData->roughness);
        gv->SetValue(groupPath, matPrefix + "Metalness", matData->metalness);
        gv->SetValue(groupPath, matPrefix + "SpecColor", matData->specularColor);
        gv->SetValue(groupPath, matPrefix + "Emissive", matData->emissiveIntensity);
        gv->SetValue(groupPath, matPrefix + "AlphaThres", matData->alphaTestThreshold);

        // シャドウ設定
        gv->SetValue(groupPath, matPrefix + "AddShadow", matData->addShadow);
        gv->SetValue(groupPath, matPrefix + "ShadowBias", matData->shadowBias);
        gv->SetValue(groupPath, matPrefix + "ShadowDens", matData->shadowDensity);
        gv->SetValue(groupPath, matPrefix + "ShadowEnv", matData->shadowEnvStrength);
        gv->SetValue(groupPath, matPrefix + "ShadowSoft", matData->shadowSoftness);

        // リムライト設定
        gv->SetValue(groupPath, matPrefix + "RimEnable", matData->enableRim);
        gv->SetValue(groupPath, matPrefix + "RimUseDir", matData->rimUseLightDir);
        gv->SetValue(groupPath, matPrefix + "RimPower", matData->rimPower);
        gv->SetValue(groupPath, matPrefix + "RimInten", matData->rimIntensity);
        gv->SetValue(groupPath, matPrefix + "RimColor", matData->rimColor);

        // ディゾルブ設定
        gv->SetValue(groupPath, matPrefix + "DisEnable", matData->enableDissolve);
        gv->SetValue(groupPath, matPrefix + "DisThres", matData->dissolveThreshold);
        gv->SetValue(groupPath, matPrefix + "EdgeWidth", matData->edgeWidth);
        gv->SetValue(groupPath, matPrefix + "EdgeInten", matData->edgeIntensity);
        gv->SetValue(groupPath, matPrefix + "EdgeColor", matData->edgeColor);

        // ノーマルマップ設定
        gv->SetValue(groupPath, matPrefix + "NormEnable", matData->enableNormalMap);
        gv->SetValue(groupPath, matPrefix + "NormTile", matData->normalTiling);
        gv->SetValue(groupPath, matPrefix + "NormInten", matData->normalIntensity);

        // POMハイトマップ設定
        gv->SetValue(groupPath, matPrefix + "POMEnable", matData->enablePOM);
        gv->SetValue(groupPath, matPrefix + "POMHeightScale", matData->pomHeightScale);
        gv->SetValue(groupPath, matPrefix + "POMMinSteps", matData->pomMinSteps);
        gv->SetValue(groupPath, matPrefix + "pomMaxSteps", matData->pomMaxSteps);

        // アウトライン設定
        gv->SetValue(groupPath, matPrefix + "OutlineEnable", matData->enableOutline);
        gv->SetValue(groupPath, matPrefix + "OutlineWidth", matData->outlineWidth);
        gv->SetValue(groupPath, matPrefix + "OutlineColor", matData->outlineColor);

        // リップル・水たまり設定
        gv->SetValue(groupPath, matPrefix + "RippleEnable", matData->enableRipple);
        gv->SetValue(groupPath, matPrefix + "UsePuddle", matData->usePuddle);
        gv->SetValue(groupPath, matPrefix + "Wetness", matData->wetness);
        gv->SetValue(groupPath, matPrefix + "PuddleEmission", matData->puddleEmission);
        gv->SetValue(groupPath, matPrefix + "RippleScale", matData->rippleScale);
        gv->SetValue(groupPath, matPrefix + "RippleSpeed", matData->rippleSpeed);
        gv->SetValue(groupPath, matPrefix + "RippleStren", matData->rippleStrength);
        gv->SetValue(groupPath, matPrefix + "PuddleScale", matData->puddleScale);
        gv->SetValue(groupPath, matPrefix + "PuddleFalloff", matData->puddleFalloff);
        gv->SetValue(groupPath, matPrefix + "RippleSize", matData->rippleSize);
        gv->SetValue(groupPath, matPrefix + "RippleFreq", matData->rippleFrequency);
        gv->SetValue(groupPath, matPrefix + "RippleMix", matData->rippleLayerMix);
        gv->SetValue(groupPath, matPrefix + "PuddleColor", matData->puddleColor);
        gv->SetValue(groupPath, matPrefix + "PuddleTint", matData->puddleTint);
    }
}


void EnvironmentProp::ChangeMasterModel(FE::Model* newMaster)
{
    masterModel_ = newMaster;

    // 現在のトランスフォーム状態を退避
    const auto currentTransform = model_->GetTransform();

    // 新しいマスターモデルに合わせたモデルインスタンスの再構築
    modelName_ = masterModel_->GetName();
    model_ = std::make_unique<FE::Model>(engine_, modelName_);

    // メッシュ・マテリアルデータの参照共有
    model_->ShareModelDataFrom(masterModel_);
    model_->ShareMaterialsFrom(masterModel_);

    // 退避データの復元
    model_->SetTransform(currentTransform);

    // インスタンス再生成に伴う PropertyBinder の再割り当て
    SetupProperties();
}