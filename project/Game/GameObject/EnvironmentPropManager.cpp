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

    // 足りない分だけ生成することで、データの意図しない初期化を防ぐ
    while (props_.size() < propCount_)
    {
        int newIndex = static_cast<int>(props_.size());
        auto prop = std::make_unique<EnvironmentProp>(engine_, newIndex, managerGroupName_);
        prop->SetManager(this->GetManager());
        prop->Initialize();

        props_.push_back(std::move(prop));
    }

    // もしJSON上の数が減っていた場合、末尾から安全に削除する
    while (props_.size() > propCount_)
    {
        props_.pop_back();
    }
}

void EnvironmentPropManager::Update()
{
    for (auto& prop : props_)
    {
        if (prop->IsActive()) prop->Update();
    }
}

void EnvironmentPropManager::Draw()
{
    for (auto& prop : props_)
    {
        if (prop->IsActive()) prop->Draw();
    }
}

void EnvironmentPropManager::AddProp()
{
    int newIndex = static_cast<int>(props_.size());
    auto newProp = std::make_unique<EnvironmentProp>(engine_, newIndex, managerGroupName_);
    newProp->SetManager(this->GetManager());
    newProp->Initialize();

    if (!props_.empty() && currentTemplateIndex_ >= 0 && currentTemplateIndex_ < props_.size())
    {
        newProp->GetModel()->CopyMaterialsFrom(props_[currentTemplateIndex_]->GetModel());
    }

    props_.push_back(std::move(newProp));
    propCount_ = static_cast<int>(props_.size());

    FE::GlobalVariables::GetInstance()->SetValue({ managerGroupName_ }, "PropCount", propCount_);
}

void EnvironmentPropManager::RemoveEnvironmentProp(int index)
{
    if (index >= 0 && index < props_.size())
    {
        // 1. JSON上の「一番最後」のデータを消去する
        // （全体の数が1つ減るため、最後尾のキーが不要になる）
        int lastIndex = static_cast<int>(props_.size()) - 1;
        std::string lastGroupName = "Prop_" + std::to_string(lastIndex);
        FE::GlobalVariables::GetInstance()->ClearGroup({ managerGroupName_, lastGroupName });

        // 2. ベクターから指定された要素を削除（ここでデストラクタが呼ばれる）
        props_.erase(props_.begin() + index);

        // 3. 削除された場所以降の要素のIDを振り直し、JSONを上書き保存させる
        for (int i = index; i < props_.size(); ++i)
        {
            props_[i]->ReassignID(i);
        }

        // 4. 全体の数を更新
        propCount_ = static_cast<int>(props_.size());
        FE::GlobalVariables::GetInstance()->SetValue({ managerGroupName_ }, "PropCount", propCount_);
    }
}

void EnvironmentPropManager::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("環境オブジェクトマネージャー");

    ImGui::Text("全体の数: %d", propCount_);
    ImGui::Separator();

    // ★ 修正: マテリアルの手動コピーUI
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "マテリアル手動コピー機能");

    // static変数にして、フレーム間でも入力値を保持する
    static int sourcePropID = 0;
    static int targetPropID = 0;

    ImGui::InputInt("コピー元 Prop ID", &sourcePropID);

    // 1. 特定のPropへコピーするボタン
    ImGui::InputInt("コピー先 Prop ID", &targetPropID);
    if (ImGui::Button("指定したPropにマテリアルをコピー"))
    {
        // 範囲外アクセス防止と、自分自身へのコピーを弾く安全対策
        if (sourcePropID >= 0 && sourcePropID < props_.size() &&
            targetPropID >= 0 && targetPropID < props_.size() &&
            sourcePropID != targetPropID)
        {
            props_[targetPropID]->GetModel()->CopyMaterialsFrom(props_[sourcePropID]->GetModel());

            // ★追加: コピーした結果を JSON のメモリデータに同期！
            props_[targetPropID]->SyncMaterialsToJSON();
        }
    }

    // 2. すべてのPropへ一括コピーするボタン
    if (ImGui::Button("すべてのPropにマテリアルをコピー"))
    {
        if (sourcePropID >= 0 && sourcePropID < props_.size())
        {
            for (int i = 0; i < props_.size(); ++i)
            {
                if (i != sourcePropID)
                {
                    props_[i]->GetModel()->CopyMaterialsFrom(props_[sourcePropID]->GetModel());

                    // ★追加: コピーした結果を JSON のメモリデータに同期！
                    props_[i]->SyncMaterialsToJSON();
                }
            }
        }
    }

    ImGui::Separator();

    // ★ 修正: AddProp() 時のコピー機能は消すため、シンプルな追加ボタンに変更
    if (ImGui::Button("新しいPropを最後尾に追加"))
    {
        AddProp();
    }

    ImGui::Separator();

    int deleteIndex = -1; // 削除予約用

    // 各Propの詳細設定
    for (int i = 0; i < props_.size(); ++i)
    {
        ImGui::PushID(i);

        // ★ 修正1: ヘッダー名に「Prop [0] : cube」などのわかりやすい名前を表示する
        std::string displayName = props_[i]->GetDisplayName();
        std::string headerName = "Prop [" + std::to_string(i) + "] : " + displayName;

        // ★ 修正2: TreeNodeに選択状態（Selectedフラグ）を持たせる
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (selectedPropIndex_ == i)
        {
            flags |= ImGuiTreeNodeFlags_Selected; // 選択中ならハイライトする
        }

        // TreeNodeExを使ってフラグを適用
        bool isOpen = ImGui::TreeNodeEx(headerName.c_str(), flags);

        // ★ 修正3: TreeNode自体（行全体）がクリックされたら、このPropを選択状態にする
        if (ImGui::IsItemClicked())
        {
            selectedPropIndex_ = i;
        }

        if (isOpen)
        {
            // ▼▼ 修正: std::string を直接いじらず、char配列を介して安全に入力させる ▼▼
            char nameBuf[256];
            // 現在の名前をバッファにコピー
            strncpy_s(nameBuf, sizeof(nameBuf), props_[i]->GetDisplayName().c_str(), _TRUNCATE);

            // 入力があった場合だけ、先ほど作った SetCustomName で更新＆保存
            if (ImGui::InputText("エディタ表示名", nameBuf, sizeof(nameBuf)))
            {
                // ここで作成した関数を呼ぶ
                props_[i]->SetCustomName(nameBuf);
            }

            props_[i]->DebugDraw();

            ImGui::Separator();

            if (ImGui::Button("このPropを削除", ImVec2(-1, 0)))
            {
                deleteIndex = i;
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    if (deleteIndex != -1)
    {
        RemoveEnvironmentProp(deleteIndex);

        // ★ 追加: 選択中のオブジェクトを消した場合は、選択状態をリセットする
        if (selectedPropIndex_ == deleteIndex)
        {
            selectedPropIndex_ = -1;
        }
        // 消した対象より後ろを選択していた場合は、インデックスがずれるので調整する
        else if (selectedPropIndex_ > deleteIndex)
        {
            selectedPropIndex_--;
        }
    }

    ImGui::End();

    // ★ 修正4: 選択されているPropが存在すれば、そのTransformに対してGizmoを描画する
    if (selectedPropIndex_ >= 0 && selectedPropIndex_ < props_.size())
    {
        // 1. ギズモ操作前のTransformを記録
        auto& targetTransform = props_[selectedPropIndex_]->GetTransformRef();
        FE::Vector3 oldPos = targetTransform.translation_;
        FE::Vector3 oldRot = targetTransform.rotation_;
        FE::Vector3 oldScale = targetTransform.scale_;

        // 2. ギズモの描画（マウス操作で targetTransform の中身が直接書き換わる）
        FE::ImGuiManager::DrawGizmo(targetTransform);

        // 3. 操作前と操作後で値が変わっているかチェック
        bool isChanged = false;
        if (oldPos.x != targetTransform.translation_.x || oldPos.y != targetTransform.translation_.y || oldPos.z != targetTransform.translation_.z) isChanged = true;
        if (oldRot.x != targetTransform.rotation_.x || oldRot.y != targetTransform.rotation_.y || oldRot.z != targetTransform.rotation_.z) isChanged = true;
        if (oldScale.x != targetTransform.scale_.x || oldScale.y != targetTransform.scale_.y || oldScale.z != targetTransform.scale_.z) isChanged = true;

        if (isChanged)
        {
            auto* gv = FE::GlobalVariables::GetInstance();

            // 例: "EnvironmentPropManager/Prop_0" のようなグループパスを生成
            std::vector<std::string> groupPath = { managerGroupName_, "Prop_" + std::to_string(selectedPropIndex_) };

            // BindModelで設定されたキー名に合わせてメモリ上のデータを更新！
            gv->SetValue(groupPath, "Model_Trans", targetTransform.translation_);
            gv->SetValue(groupPath, "Model_Rot", targetTransform.rotation_);
            gv->SetValue(groupPath, "Model_Scale", targetTransform.scale_);
        }
    }

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