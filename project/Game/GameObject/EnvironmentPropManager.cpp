#include "pch.h"
#include "EnvironmentPropManager.h"
#include "GlobalVariables.h"
#include "ImGuiManager.h"

EnvironmentPropManager::EnvironmentPropManager(FE::Engine* engine, const std::string& groupName)
    : engine_(engine), managerGroupName_(groupName)
{}

void EnvironmentPropManager::CreateGroup(const std::string& prefabName, const std::string& fallbackModelName)
{
    if (groups_.find(prefabName) != groups_.end()) return;

    // ★ 修正: 先に map 内に要素を作成し、その参照を取得する
    auto& newGroup = groups_[prefabName];
    newGroup.prefabName = prefabName;

    // マスターの設定を保存するJSONグループ名
    std::string masterGroupName = "Master_" + prefabName;
    auto* gv = FE::GlobalVariables::GetInstance();

    // JSONから「このプレハブがどの3Dモデルを使うか」を取得
    std::string loadedModel = gv->GetStringValue({ managerGroupName_, masterGroupName }, "ModelName");
    if (!loadedModel.empty()) {
        newGroup.modelName = loadedModel;
    }
    else {
        newGroup.modelName = fallbackModelName;
        // 新規作成時はJSONにモデル名を書き込む
        gv->SetValue({ managerGroupName_, masterGroupName }, "ModelName", newGroup.modelName);
    }

    newGroup.masterModel = std::make_unique<FE::Model>(engine_, newGroup.modelName);

    // マスター専用のバインダー
    newGroup.binder = std::make_unique<FE::PropertyBinder>(engine_, managerGroupName_, masterGroupName);

    // ★ 修正: map内に確定した &newGroup.modelName の安全なアドレスが渡される
    newGroup.binder->BindModelName("ModelName", &newGroup.modelName, newGroup.modelName,
        [this, prefabName](const std::string& newName) {
            // ImGui描画中での即時再構築を避けるため、予約変数に保存
            this->pendingModelChangePrefab_ = prefabName;
            this->pendingModelChangeNewName_ = newName;
        });

    newGroup.binder->BindModel("MasterModel", newGroup.masterModel.get());
}

void EnvironmentPropManager::Initialize()
{
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, managerGroupName_);
    binder_->Bind("PropCount", &propCount_, 0);

    auto* gv = FE::GlobalVariables::GetInstance();

    for (int i = 0; i < propCount_; ++i)
    {
        std::string childGroupName = "Prop_" + std::to_string(i);

        // ★変更: 「ModelName」ではなく「PrefabName」を読み込む
        std::string prefabName = gv->GetStringValue({ managerGroupName_, childGroupName }, "PrefabName");
        std::string fallbackModel = prefabName;

        if (prefabName.empty()) {
            // ※以前のセーブデータとの互換性対応
            prefabName = gv->GetStringValue({ managerGroupName_, childGroupName }, "ModelName");
            if (prefabName.empty()) prefabName = "cube";
            fallbackModel = prefabName;
        }

        // グループが無ければ作成（既存プレハブの場合はフォールバックは使われない）
        CreateGroup(prefabName, fallbackModel);

        auto prop = std::make_unique<EnvironmentProp>(engine_, i, managerGroupName_);
        prop->SetPrefabName(prefabName); // ★追加
        prop->SetMasterModel(groups_[prefabName].masterModel.get());
        prop->SetManager(this->GetManager());
        prop->Initialize();

        groups_[prefabName].instances.push_back(std::move(prop));
    }
}

void EnvironmentPropManager::Update()
{
    // すべてのグループの実体をUpdate
    for (auto& [prefabName, group] : groups_) {
        for (auto& prop : group.instances) {
            if (prop->IsActive()) prop->Update();
        }
    }
}

void EnvironmentPropManager::Draw()
{
    for (auto& [prefabName, group] : groups_) {
        for (auto& prop : group.instances) {
            if (prop->IsActive()) prop->Draw();
        }
    }
}

void EnvironmentPropManager::AddPropToGroup(const std::string& prefabName)
{
    int newIndex = propCount_;

    // グループが無いことはないはずだが念のため
    CreateGroup(prefabName, "cube");

    auto newProp = std::make_unique<EnvironmentProp>(engine_, newIndex, managerGroupName_);
    newProp->SetPrefabName(prefabName); // ★追加
    newProp->SetMasterModel(groups_[prefabName].masterModel.get());
    newProp->SetManager(this->GetManager());
    newProp->Initialize();

    // ★変更: 「ModelName」ではなく「PrefabName」を保存
    FE::GlobalVariables::GetInstance()->SetValue({ managerGroupName_, "Prop_" + std::to_string(newIndex) }, "PrefabName", prefabName);

    groups_[prefabName].instances.push_back(std::move(newProp));

    propCount_++;
    FE::GlobalVariables::GetInstance()->SetValue({ managerGroupName_ }, "PropCount", propCount_);
}

void EnvironmentPropManager::RemoveEnvironmentProp(int targetId)
{
    // 1. JSON上の一番最後のデータを消去
    int lastIndex = propCount_ - 1;
    std::string lastGroupName = "Prop_" + std::to_string(lastIndex);
    FE::GlobalVariables::GetInstance()->ClearGroup({ managerGroupName_, lastGroupName });

    // 2. targetId を持つインスタンスを探して削除
    for (auto& [prefabName, group] : groups_) {
        auto it = std::remove_if(group.instances.begin(), group.instances.end(),
            [targetId](const std::unique_ptr<EnvironmentProp>& p) { return p->GetID() == targetId; });

        if (it != group.instances.end()) {
            group.instances.erase(it, group.instances.end());
            break;
        }
    }

    // 3. 削除したIDより大きいIDを持つプロップのIDを -1 して詰める
    for (auto& [prefabName, group] : groups_) {
        for (auto& prop : group.instances) {
            if (prop->GetID() > targetId) {
                prop->ReassignID(prop->GetID() - 1);

                // ★ 変更: "ModelName" ではなく "PrefabName" として保存し直す
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

    // もし削除したプロップを選択中だったら解除
    if (selectedProp_ != nullptr && selectedProp_->GetID() == targetId) {
        selectedProp_ = nullptr;
    }
}

void EnvironmentPropManager::ExecutePrefabModelChange()
{
    if (pendingModelChangePrefab_.empty() || pendingModelChangeNewName_.empty()) return;

    auto it = groups_.find(pendingModelChangePrefab_);
    if (it != groups_.end())
    {
        auto& group = it->second;
        group.modelName = pendingModelChangeNewName_;

        // マスターモデルを作り直す
        group.masterModel = std::make_unique<FE::Model>(engine_, group.modelName);

        // バインダーをクリアして再登録（古いモデルへのポインタを無効化するため）
        group.binder->Clear();

        std::string targetPrefab = group.prefabName;
        group.binder->BindModelName("ModelName", &group.modelName, group.modelName,
            [this, targetPrefab](const std::string& newName) {
                this->pendingModelChangePrefab_ = targetPrefab;
                this->pendingModelChangeNewName_ = newName;
            });

        group.binder->BindModel("MasterModel", group.masterModel.get());

        // このPrefabに属するすべてのインスタンスに新しいマスターを適用
        for (auto& prop : group.instances)
        {
            prop->ChangeMasterModel(group.masterModel.get());
        }
    }

    // 予約をクリア
    pendingModelChangePrefab_ = "";
    pendingModelChangeNewName_ = "";
}

void EnvironmentPropManager::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("環境オブジェクトマネージャー");

    int deleteRequestID = -1; // 削除予約用

    for (auto& [prefabName, group] : groups_)
    {
        ImGui::PushID(prefabName.c_str());

        std::string groupHeader = prefabName + " (" + std::to_string(group.instances.size()) + "個)";

        if (ImGui::CollapsingHeader(groupHeader.c_str()))
        {
            ImGui::Indent();

            if (ImGui::TreeNode("共通マテリアル設定 (Prefab)"))
            {
                group.binder->Draw("ModelName", "ベース3Dモデル");

                // バインダーを経由してマスターモデルを描画
                bool isMaterialChanged = group.binder->DrawModel("MasterModel", "マテリアル");

                // 変更を検知したら即座に適用
                if (isMaterialChanged)
                {
                    for (auto& inst : group.instances) {
                        inst->GetModel()->ShareMaterialsFrom(group.masterModel.get());
                    }
                }
                ImGui::TreePop();
            }

            ImGui::Separator();
            ImGui::Text("配置済みインスタンス一覧");

            for (int i = 0; i < group.instances.size(); ++i)
            {
                auto& prop = group.instances[i];
                ImGui::PushID(prop->GetID());

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
                if (selectedProp_ == prop.get()) flags |= ImGuiTreeNodeFlags_Selected;

                std::string instName = "Prop [" + std::to_string(prop->GetID()) + "] : " + prop->GetDisplayName();
                bool isOpen = ImGui::TreeNodeEx(instName.c_str(), flags);

                if (ImGui::IsItemClicked()) {
                    selectedProp_ = prop.get();
                }

                if (isOpen)
                {
                    prop->DebugDraw();

                    if (ImGui::Button("このPropを削除", ImVec2(-1, 0))) {
                        deleteRequestID = prop->GetID();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            if (ImGui::Button("+ この種類のPropを追加", ImVec2(-1, 0)))
            {
                AddPropToGroup(prefabName);
            }

            ImGui::Unindent();
        }
        ImGui::PopID();
    }

    // 削除処理の実行
    if (deleteRequestID != -1) {
        RemoveEnvironmentProp(deleteRequestID);
    }

    ImGui::Separator();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "新しいPrefabの作成");
    static char newPrefabName[128] = "MyNewPrefab";
    static char baseModelName[128] = "cube";
    ImGui::InputText("Prefab名", newPrefabName, sizeof(newPrefabName));
    ImGui::InputText("ベースの3Dモデル名", baseModelName, sizeof(baseModelName));

    if (ImGui::Button("この設定でPrefabを作成", ImVec2(-1, 0))) {
        CreateGroup(newPrefabName, baseModelName);
    }

    ImGui::End();

    // ==========================================
    // ギズモ描画 (ポインタ基準に変更)
    // ==========================================
    if (selectedProp_ != nullptr)
    {
        auto& targetTransform = selectedProp_->GetTransformRef();
        FE::Vector3 oldPos = targetTransform.translation_;
        FE::Vector3 oldRot = targetTransform.rotation_;
        FE::Vector3 oldScale = targetTransform.scale_;

        FE::ImGuiManager::DrawGizmo(targetTransform);

        bool isChanged = false;
        if (oldPos.x != targetTransform.translation_.x || oldPos.y != targetTransform.translation_.y || oldPos.z != targetTransform.translation_.z) isChanged = true;
        if (oldRot.x != targetTransform.rotation_.x || oldRot.y != targetTransform.rotation_.y || oldRot.z != targetTransform.rotation_.z) isChanged = true;
        if (oldScale.x != targetTransform.scale_.x || oldScale.y != targetTransform.scale_.y || oldScale.z != targetTransform.scale_.z) isChanged = true;

        if (isChanged)
        {
            auto* gv = FE::GlobalVariables::GetInstance();
            std::vector<std::string> groupPath = { managerGroupName_, "Prop_" + std::to_string(selectedProp_->GetID()) };

            gv->SetValue(groupPath, "Position", targetTransform.translation_);
            gv->SetValue(groupPath, "Rotation", targetTransform.rotation_);
            gv->SetValue(groupPath, "Scale", targetTransform.scale_);
        }
    }

    ExecutePrefabModelChange();

#endif
}

void EnvironmentProp::SyncMaterialsToJSON()
{
    auto* gv = FE::GlobalVariables::GetInstance();

    // PropertyBinder::BindModel() で指定しているグループパスを取得
    // ※ binder_ は EnvironmentProp クラス内にある PropertyBinder のインスタンスを想定しています
    const std::vector<std::string>& groupPath = binder_->GetGroupPath();

    // ベースとなる接頭辞 (BindModel() で "Model_" にしているため)
    std::string prefix = "Model_";

    size_t count = model_->GetMaterialCount();
    for (size_t i = 0; i < count; ++i)
    {
        // マテリアルが1つなら "Model_"、複数なら "Model_Mat0_" などのプレフィックスを作成
        std::string matPrefix = (count == 1) ? prefix : prefix + "Mat" + std::to_string(i) + "_";

        FE::MaterialHandle* handle = model_->GetMaterialHandle(i);
        MaterialData* matData = model_->GetMaterialData(i);

        if (!handle || !matData) continue;

        // ---------------------------------------------------------
        // 1. テクスチャ名の同期 (MaterialHandle から)
        // ---------------------------------------------------------
        gv->SetValue(groupPath, matPrefix + "AlbedoMap", handle->textureName);
        gv->SetValue(groupPath, matPrefix + "EnvMapTex", handle->envMapName);
        gv->SetValue(groupPath, matPrefix + "NormalMapTex", handle->normalMapName);
        gv->SetValue(groupPath, matPrefix + "HeightMapTex", handle->heightMapName);
        gv->SetValue(groupPath, matPrefix + "DissolveTex", handle->dissolveMapName);
        gv->SetValue(groupPath, matPrefix + "ToonRampTex", handle->toonRampName);
        gv->SetValue(groupPath, matPrefix + "RippleMap", handle->rippleTextureName);
        gv->SetValue(groupPath, matPrefix + "PuddleNoise", handle->puddleNoiseName);

        // ---------------------------------------------------------
        // 2. UVトランスフォームの同期
        // ---------------------------------------------------------
        gv->SetValue(groupPath, matPrefix + "UVTrans", handle->uvTransformData.translation_);
        gv->SetValue(groupPath, matPrefix + "UVRot", handle->uvTransformData.rotation_);
        gv->SetValue(groupPath, matPrefix + "UVScale", handle->uvTransformData.scale_);

        // ---------------------------------------------------------
        // 3. マテリアルプロパティの同期 (MaterialData から)
        // ---------------------------------------------------------
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

    // 現在の座標・回転・スケールを退避
    auto currentTransform = model_->GetTransform();

    // モデルを再生成
    modelName_ = masterModel_->GetName();
    model_ = std::make_unique<FE::Model>(engine_, modelName_);

    // 新しいマスターモデルからデータとマテリアルを共有
    model_->ShareModelDataFrom(masterModel_);
    model_->ShareMaterialsFrom(masterModel_);

    // 退避したトランスフォームを復元
    model_->SetTransform(currentTransform);

    // モデルが再生成されポインタアドレスが変わったため、
    // PropertyBinder の登録をすべてやり直す
    SetupProperties();
}