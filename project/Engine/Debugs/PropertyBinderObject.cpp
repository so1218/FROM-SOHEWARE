#include "pch.h"
#include "PropertyBinder.h"
#include "Model.h"
#include "AnimationModel.h"
#include "Sprite.h"
#include "SRVManager.h"
#include "Terrain.h"
#include "Engine.h"

namespace FE
{

void PropertyBinder::BindModel(const std::string& groupName, Model* model)
{
    modelBindMap_[groupName] = { model };

    auto* transform = &model->GetTransform();
    std::string prefix = groupName + "_";

    Bind(prefix + "Trans", &transform->translation_, { 0.0f, 0.0f, 0.0f }, 0.1f);
    BindRotation(prefix + "Rot", &transform->rotation_, &transform->rotationQuaternion_, 0.01f);
    Bind(prefix + "Scale", &transform->scale_, { 1.0f, 1.0f, 1.0f }, 0.1f);

    size_t matCount = model->GetMaterialCount();
    for (size_t i = 0; i < matCount; ++i)
    {
        // マテリアルが1つなら Mat0 を省略、複数なら Mat0_, Mat1_ と付与
        std::string matPrefix = (matCount == 1) ? prefix : prefix + "Mat" + std::to_string(i) + "_";
        BindMaterialProperties(matPrefix, model->GetMaterialHandle(i));
    }
}

void PropertyBinder::BindAnimationModel(const std::string& groupName, AnimationModel* model)
{
    animationBindMap_[groupName] = { model };

    auto* transform = &model->GetTransform();
    std::string prefix = groupName + "_";

    // アニメーション固有の設定
    Bind(prefix + "SpeedScale", model->GetSpeedScalePtr(), 1.0f, 0.1f, 0.0f, 5.0f);
    Bind(prefix + "IsLoop", model->GetIsLoopPtr(), true);

    Bind(prefix + "Trans", &transform->translation_, { 0.0f, 0.0f, 0.0f }, 0.1f);
    BindRotation(prefix + "Rot", &transform->rotation_, &transform->rotationQuaternion_, 0.01f);
    Bind(prefix + "Scale", &transform->scale_, { 1.0f, 1.0f, 1.0f }, 0.1f);

    size_t matCount = model->GetMaterialCount();
    for (size_t i = 0; i < matCount; ++i) {
        std::string matPrefix = (matCount == 1) ? prefix : prefix + "Mat" + std::to_string(i) + "_";
        BindMaterialProperties(matPrefix, model->GetMaterialHandle(i));
    }
}

void PropertyBinder::BindModelName(
    const std::string& key,
    std::string* currentModelName,
    const std::string& defaultName,
    std::function<void(const std::string&)> onChange)
{
    auto* gv = GlobalVariables::GetInstance();

    // ロード処理または初期値の設定
    std::string loadedName = gv->GetStringValue(groupPath_, key);
    if (loadedName.empty()) {
        *currentModelName = defaultName;
        gv->SetValue(groupPath_, key, defaultName);
    }
    else {
        *currentModelName = loadedName;
    }

    keys_.push_back(key);

#ifdef ENABLE_IMGUI
    items_[key] = [this, currentModelName, onChange, key](const std::string& nameOverride) {
        std::vector<std::string> modelNames = ModelManager::GetInstance().GetLoadedModelNames();
        if (modelNames.empty()) return false;

        // 現在のモデル名が何番目にあるか検索
        int currentIndex = 0;
        for (int i = 0; i < modelNames.size(); ++i) {
            if (modelNames[i] == *currentModelName) {
                currentIndex = i;
                break;
            }
        }

        // C文字列ポインタの配列を構築
        std::vector<const char*> items(modelNames.size());
        for (size_t i = 0; i < modelNames.size(); ++i) {
            items[i] = modelNames[i].c_str();
        }

        std::string label = MakeLabel(key, nameOverride);

        if (ImGui::Combo(label.c_str(), &currentIndex, items.data(), static_cast<int>(items.size()))) {
            *currentModelName = modelNames[currentIndex];
            GlobalVariables::GetInstance()->SetValue(groupPath_, key, *currentModelName);

            if (onChange) {
                onChange(*currentModelName);
            }
            return true;
        }
        return false;
        };
#endif
}

bool PropertyBinder::DrawModel(const std::string& groupName, const std::string& customLabel)
{
#ifdef ENABLE_IMGUI
    std::string prefix = groupName + "_";
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    // マップに存在すればモデルを取得
    Model* targetModel = modelBindMap_.count(groupName) ? modelBindMap_[groupName].model : nullptr;
    bool isChanged = false;

    ImGui::PushID(groupName.c_str());
    if (ImGui::CollapsingHeader(displayLabel.c_str()))
    {
        ImGui::Spacing();
        ImGui::SeparatorText("トランスフォーム");
        isChanged |= Draw(prefix + "Trans", "位置");
        isChanged |= Draw(prefix + "Rot", "回転");
        isChanged |= Draw(prefix + "Scale", "スケール");

        ImGui::Spacing();
        isChanged |= DrawMaterialUI(targetModel, prefix, GlobalVariables::GetInstance(), groupPath_);
    }
    ImGui::PopID();

    return isChanged;
#else
    return false;
#endif
}

void PropertyBinder::DrawAnimationModel(const std::string& groupName, const std::string& customLabel)
{
#ifdef ENABLE_IMGUI
    std::string prefix = groupName + "_";
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    AnimationModel* targetModel = animationBindMap_.count(groupName) ? animationBindMap_[groupName].model : nullptr;

    ImGui::PushID(groupName.c_str());
    if (ImGui::CollapsingHeader(displayLabel.c_str())) {
        ImGui::Spacing();
        ImGui::SeparatorText("アニメーション設定");
        Draw(prefix + "SpeedScale", "再生速度");
        Draw(prefix + "IsLoop", "ループ再生");

        ImGui::Spacing();
        ImGui::SeparatorText("トランスフォーム");
        Draw(prefix + "Trans", "位置");
        Draw(prefix + "Rot", "回転");
        Draw(prefix + "Scale", "スケール");

        ImGui::Spacing();
        DrawMaterialUI(targetModel, prefix, GlobalVariables::GetInstance(), groupPath_);
    }
    ImGui::PopID();
#endif
}

void PropertyBinder::BindSprite(const std::string& groupName, Sprite* sprite)
{
    auto* mat = sprite->GetMaterial();
    auto& uvTransform = sprite->GetUVTransform();

    std::string prefix = groupName + "_";

    Bind(prefix + "Pos", &sprite->GetPosition(), { 0.0f, 0.0f }, 1.0f);
    Bind(prefix + "Size", &sprite->GetSize(), { 100.0f, 100.0f }, 1.0f);
    Bind(prefix + "Rot", &sprite->GetRotation(), 0.0f, 0.01f);
    Bind(prefix + "Anchor", sprite->GetAnchorPointPtr(), { 0.0f, 0.0f }, 0.01f);

    BindColor(prefix + "Color", sprite->GetColorPtr(), 0xFFFFFFFF);

    // メインテクスチャ
    BindTexture(
        prefix + "Tex",                 // キー
        sprite->GetTextureName(),       // 初期値
        [sprite](const std::string& newName)// 変更時の処理
        {
            sprite->SetTexture(newName);
        },
        "white1x1",                     // デフォルト名
        TextureType::UI             // フィルタ
    );

    Bind(prefix + "Visible", sprite->GetIsVisiblePtr(), true);
    Bind(prefix + "Layer", sprite->GetLayerOrderPtr(), 0, 1.0f);

    auto onUVChange = [sprite]() {
        sprite->UpdateUV();
        };
    Bind(prefix + "UVTrans", &uvTransform.translation_, { 0.0f, 0.0f, 0.0f }, 0.01f, onUVChange);
    BindRotation(prefix + "UVRot", &uvTransform.rotation_, &uvTransform.rotationQuaternion_, 0.01f);
    Bind(prefix + "UVScale", &uvTransform.scale_, { 1.0f, 1.0f, 1.0f }, 0.01f);

    BindBool(prefix + "DisEnable", &mat->enableDissolve, false);

    // ディゾルブテクスチャ
    BindTexture(
        prefix + "DisTex",
        sprite->GetDissolveTextureName(),
        [sprite](const std::string& newName) {
            sprite->SetDissolveTexture(newName);
        },
        "white1x1",
        TextureType::Noise // ノイズ用フィルタ
    );

    Bind(prefix + "DisThres", &mat->dissolveThreshold, 0.5f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "EdgeWidth", &mat->edgeWidth, 0.05f, 0.001f, 0.0f, 0.5f);
    Bind(prefix + "EdgeInten", &mat->edgeIntensity, 2.0f, 0.1f, 0.0f, 10.0f);
    BindColor(prefix + "EdgeColor", &mat->edgeColor, { 1.0f, 0.5f, 0.0f });
}

void PropertyBinder::DrawSprite(const std::string & groupName, const std::string & customLabel)
{
#ifdef ENABLE_IMGUI
    std::string prefix = groupName + "_";
    auto* gv = GlobalVariables::GetInstance();
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    ImGui::PushID(groupName.c_str());

    bool isOpened = ImGui::CollapsingHeader(displayLabel.c_str(), ImGuiTreeNodeFlags_None);

    if (isOpened)
    {
        ImGui::Indent(12.0f);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);

        ImGui::Spacing();
        ImGui::SeparatorText("基本設定");
        Draw(prefix + "Visible", "表示");
        Draw(prefix + "Layer", "描画順");

        ImGui::Spacing();
        ImGui::SeparatorText("トランスフォーム");
        Draw(prefix + "Pos", "位置");
        Draw(prefix + "Size", "スケール");
        Draw(prefix + "Rot", "回転");
        Draw(prefix + "Anchor", "アンカーポイント");

        ImGui::Spacing();
        ImGui::SeparatorText("見た目");
        Draw(prefix + "Color", "カラー");
        Draw(prefix + "Tex", "メインテクスチャ");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (ImGui::TreeNodeEx("UVTransform", nodeFlags, "UV トランスフォーム"))
        {
            ImGui::Spacing();
            Draw(prefix + "UVTrans", "UV 位置");
            Draw(prefix + "UVRot", "UV 回転");
            Draw(prefix + "UVScale", "UV スケール");
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Dissolve", nodeFlags, "ディゾルブ"))
        {
            ImGui::Spacing();
            Draw(prefix + "DisEnable", "有効化");

            if (gv->GetIntValue(groupPath_, prefix + "DisEnable") > 0)
            {
                ImGui::Indent(10.0f);
                Draw(prefix + "DisTex", "ノイズマップ");
                Draw(prefix + "DisThres", "進行度");

                ImGui::Separator();
                ImGui::TextDisabled("エッジ設定");
                Draw(prefix + "EdgeWidth", "エッジ幅");
                Draw(prefix + "EdgeInten", "エッジ発光強度");
                Draw(prefix + "EdgeColor", "エッジ色");
                ImGui::Unindent(10.0f);
            }
            ImGui::TreePop();
        }

        ImGui::PopItemWidth();
        ImGui::Unindent(12.0f);
    }

    ImGui::PopID();
#endif
}

void PropertyBinder::BindTexture(
    const std::string& key,
    const std::string& initialValue,
    std::function<void(const std::string&)> onValueChanged,
    const std::string& defaultName,
    TextureType filterType)
{
    auto* gv = GlobalVariables::GetInstance();

    // デフォルト値の決定と登録
    std::string valToSave = initialValue.empty() ? defaultName : initialValue;
    gv->AddItem(groupPath_, key, valToSave);

    // 保存されている値を取得し、コールバックを実行（
    std::string savedValue = gv->GetStringValue(groupPath_, key);
    if (onValueChanged) 
    {
        onValueChanged(savedValue);
    }

#ifdef ENABLE_IMGUI
    // 描画処理の登録
    items_[key] = [this, key, filterType, onValueChanged](const std::string& nameOverride) 
        {
        auto& texManager = TextureManager::GetInstance();
        auto* srvManager = this->engine_->GetSRVManager();
        auto* gv = GlobalVariables::GetInstance();

        std::string currentName = gv->GetStringValue(groupPath_, key);
        uint32_t currentHandle = texManager.Get(currentName);
        auto currentGpuHandle = srvManager->GetSRVHandleGPU(currentHandle);
        const auto* currentMeta = texManager.GetMetaData(currentName);

        bool isCubeMap = (currentMeta && currentMeta->type == TextureType::CubeMap);
        bool valueChanged = false;
        std::string newName = currentName;

        // ラベル表示
        std::string displayLabel = nameOverride.empty() ? key : nameOverride;
        ImGui::Text("%s", displayLabel.c_str());

        // プレビューボタン
        bool openPopup = false;
        ImVec2 previewSize(64.0f, 64.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        if (isCubeMap)
        {
            if (ImGui::Button("CUBE\nMAP", previewSize)) openPopup = true;
        }
        else if (currentGpuHandle.ptr != 0)
        {
            if (ImGui::ImageButton(key.c_str(), (ImTextureID)currentGpuHandle.ptr, previewSize)) openPopup = true;
        }
        else 
        {
            if (ImGui::Button("Null", previewSize)) openPopup = true;
        }
        ImGui::PopStyleColor();

        // ツールチップ
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Name: %s", currentName.c_str());
            if (!isCubeMap && currentGpuHandle.ptr != 0)
            {
                ImGui::Image((ImTextureID)currentGpuHandle.ptr, ImVec2(128.0f, 128.0f));
            }
            ImGui::EndTooltip();
        }

        ImGui::SameLine();

        // テキストボックス入力
        char buffer[256];
        strncpy_s(buffer, currentName.c_str(), _TRUNCATE);

        if (ImGui::InputText(("##TexName_" + key).c_str(), buffer, sizeof(buffer)))
        {
            newName = buffer;
            valueChanged = true;
        }

        // 選択パレット
        std::string popupId = "Popup_" + key;
        if (openPopup) 
        {
            ImGui::OpenPopup(popupId.c_str());
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(300, 200), ImVec2(800, 600));
        if (ImGui::BeginPopup(popupId.c_str())) 
        {
            const auto& allTextures = texManager.GetAllTextures();
            ImVec2 btnSize(48.0f, 48.0f);
            float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
            int displayedCount = 0;

            for (size_t i = 0; i < allTextures.size(); i++) 
            {
                const auto& data = allTextures[i];
                if (data.type != filterType) continue; // フィルタに合わないものはスキップ

                auto hGPU = srvManager->GetSRVHandleGPU(data.handle);
                bool isSelected = (currentName == data.name);
                bool isItemCube = (data.type == TextureType::CubeMap);

                ImGui::PushID(static_cast<int>(i));
                ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

                bool clicked = false;
                if (isItemCube) 
                {
                    if (ImGui::Button("CUBE", btnSize)) clicked = true;
                }
                else 
                {
                    if (ImGui::ImageButton("Tex", (ImTextureID)hGPU.ptr, btnSize)) clicked = true;
                }
                ImGui::PopStyleColor();

                if (clicked) 
                {
                    newName = data.name;
                    valueChanged = true;
                    ImGui::CloseCurrentPopup();
                }

                // ポップアップ内ツールチップ
                if (ImGui::IsItemHovered()) 
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Name: %s", data.name.c_str());
                    ImGui::Separator();
                    if (!isItemCube && hGPU.ptr != 0) {
                        ImGui::Image((ImTextureID)hGPU.ptr, ImVec2(128.0f, 128.0f));
                    }
                    ImGui::EndTooltip();
                }
                ImGui::PopID();

                // 折り返し処理
                float nextBtnX2 = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + btnSize.x;
                if (nextBtnX2 < windowVisibleX2) 
                {
                    ImGui::SameLine();
                }
                displayedCount++;
            }

            if (displayedCount == 0) ImGui::TextDisabled("No textures found.");
            ImGui::EndPopup();
        }

        // 値の更新と通知
        if (valueChanged)
        {
            gv->SetValue(groupPath_, key, newName);
            if (onValueChanged) {
                onValueChanged(newName);
            }
            return true;
        }
        return false;
        };
#endif
}

void PropertyBinder::BindTexture(
    const std::string& key,
    std::string* currentNamePtr,
    uint32_t* currentHandlePtr,
    const std::string& defaultName,
    TextureType filterType,
    std::function<void()> callback)
{
    if (!currentNamePtr || !currentHandlePtr) return;

    BindTexture(key, *currentNamePtr,
        [currentNamePtr, currentHandlePtr, callback](const std::string& newName) 
        {
            *currentNamePtr = newName;
            *currentHandlePtr = TextureManager::GetInstance().Get(newName);

            if (callback) {
                callback();
            }
        },
        defaultName,
        filterType
    );
}

void PropertyBinder::BindTerrain(const std::string& groupName, Terrain* terrain)
{
    if (!terrain) return;

    terrainBindMap_[groupName] = { terrain };
    std::string prefix = groupName + "_";

    Bind(prefix + "Position", &terrain->GetTransform().translation_, { 0.0f, 0.0f, 0.0f }, 1.0f);

    // ハイトマップ画像変更時のコールバックを直接渡す
    BindTexture(prefix + "Heightmap", terrain->GetHeightmapNamePtr(), terrain->GetHeightmapHandlePtr(), "white1x1", TextureType::Noise,
        [terrain]() 
        {
            terrain->LoadFromHeightmap(terrain->GetHeightmapName(), terrain->GetParams().cellSize);
        }
    );

    // 最大高さ変更時にメッシュを再生成するコールバックを直接渡す
    Bind(prefix + "MaxHeight", &terrain->GetParams().maxHeight, 20.0f, 0.1f, 0.0f, 500.0f,
        [terrain]() 
        {
            terrain->RebuildMesh();
        }
    );

    BindMaterialProperties(prefix, terrain->GetMaterialHandle());
}

void PropertyBinder::DrawTerrain(const std::string& groupName, const std::string& customLabel)
{
#ifdef ENABLE_IMGUI
    std::string prefix = groupName + "_";
    auto* gv = GlobalVariables::GetInstance();
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    Terrain* targetTerrain = nullptr;
    if (terrainBindMap_.find(groupName) != terrainBindMap_.end()) {
        targetTerrain = terrainBindMap_[groupName].terrain;
    }

    ImGui::PushID(groupName.c_str());
    if (ImGui::CollapsingHeader(displayLabel.c_str()))
    {
        ImGui::Spacing();
        ImGui::SeparatorText("地形パラメータ");

        Draw(prefix + "Position", "地形の位置");

        Draw(prefix + "Heightmap", "ハイトマップ画像");

        Draw(prefix + "MaxHeight", "地形の最大高さ");

        ImGui::Spacing();
        DrawMaterialUI(targetTerrain, prefix, gv, groupPath_);
    }
    ImGui::PopID();
#endif
}

// マテリアルのプロパティを登録する関数
void PropertyBinder::BindMaterialProperties(const std::string& prefix, MaterialHandle* handle)
{
    if (!handle || !handle->materialData) return;

    MaterialData* matData = handle->materialData;

    BindTexture(prefix + "AlbedoMap", &handle->textureName, &handle->textureHandle, "white1x1", TextureType::Albedo);
    BindTexture(prefix + "EnvMapTex", &handle->envMapName, &handle->envMapHandle, "pureSky", TextureType::CubeMap);
    BindTexture(prefix + "NormalMapTex", &handle->normalMapName, &handle->normalMapHandle, "normal_01", TextureType::Normal);
    BindTexture(prefix + "DissolveTex", &handle->dissolveMapName, &handle->dissolveMapHandle, "white1x1", TextureType::Noise);
    BindTexture(prefix + "ToonRampTex", &handle->toonRampName, &handle->toonRampHandle, "toonRamp_01", TextureType::Toon);
    BindTexture(prefix + "RippleMap", &handle->rippleTextureName, &handle->rippleTextureHandle, "normal_00", TextureType::Normal);

    auto onUVChange = [handle]()
        {
            handle->uvTransformData.rotationQuaternion_ = Quaternion::QuaternionFromEuler(handle->uvTransformData.rotation_);
            Matrix4x4 mat = Matrix4x4::MakeAffine(
                handle->uvTransformData.scale_,
                handle->uvTransformData.rotationQuaternion_,
                handle->uvTransformData.translation_
            );
            // 結果をConstantBufferに書き込む
            handle->materialData->uvTransform = mat;
        };

    Bind(prefix + "UVTrans", &handle->uvTransformData.translation_, { 0.0f, 0.0f, 0.0f }, 0.01f, onUVChange);
    Bind(prefix + "UVRot", &handle->uvTransformData.rotation_, { 0.0f, 0.0f, 0.0f }, 0.01f, onUVChange);
    Bind(prefix + "UVScale", &handle->uvTransformData.scale_, { 1.0f, 1.0f, 1.0f }, 0.01f, onUVChange);

    BindColor(prefix + "Color", &matData->color, { 1.0f, 1.0f, 1.0f, 1.0f });
    BindBool(prefix + "Lighting", &matData->enableLighting, true);
    BindCombo(prefix + "LightMode", &matData->lightMode, 1, "ハーフランバート\0スペキュラ\0トゥーン\0PBR\0");
    Bind(prefix + "DiffuseRef", &matData->diffuseReflection, 4.0f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "Shininess", &matData->shininess, 50.0f, 0.1f, 1.0f, 256.0f);
    Bind(prefix + "EnvMapInt", &matData->environmentMapIntensity, 0.0f, 0.01f, 0.0f, 10.0f);

    Bind(prefix + "Roughness", &matData->roughness, 0.5f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "Metalness", &matData->metalness, 0.0f, 0.01f, 0.0f, 1.0f);
    BindColor(prefix + "SpecColor", &matData->specularColor, { 1.0f, 1.0f, 1.0f, 1.0f });
    Bind(prefix + "Emissive", &matData->emissiveIntensity, 1.0f, 0.1f, 0.0f, 100.0f);
    Bind(prefix + "AlphaThres", &matData->alphaTestThreshold, 0.01f, 0.01f, 0.0f, 1.0f);

    BindBool(prefix + "AddShadow", &matData->addShadow, true);
    Bind(prefix + "ShadowBias", &matData->shadowBias, 0.0005f, 0.0001f, 0.0f, 0.1f);
    Bind(prefix + "ShadowDens", &matData->shadowDensity, 0.8f, 0.01f, 0.0f, 0.99f);
    Bind(prefix + "ShadowEnv", &matData->shadowEnvStrength, 0.0f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "ShadowSoft", &matData->shadowSoftness, 0.0f, 0.01f, 0.0f, 5.0f);

    BindBool(prefix + "RimEnable", &matData->enableRim, false);
    BindBool(prefix + "RimUseDir", &matData->rimUseLightDir, false);
    Bind(prefix + "RimPower", &matData->rimPower, 3.0f, 0.1f, 0.0f, 20.0f);
    Bind(prefix + "RimInten", &matData->rimIntensity, 1.0f, 0.1f, 0.0f, 10.0f);
    BindColor(prefix + "RimColor", &matData->rimColor, { 1.0f, 1.0f, 1.0f });

    BindBool(prefix + "DisEnable", &matData->enableDissolve, false);
    Bind(prefix + "DisThres", &matData->dissolveThreshold, 0.5f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "EdgeWidth", &matData->edgeWidth, 0.05f, 0.001f, 0.0f, 0.5f);
    Bind(prefix + "EdgeInten", &matData->edgeIntensity, 2.0f, 0.1f, 0.0f, 10.0f);
    BindColor(prefix + "EdgeColor", &matData->edgeColor, { 1.0f, 0.5f, 0.0f });

    BindBool(prefix + "NormEnable", &matData->enableNormalMap, false);
    Bind(prefix + "NormTile", &matData->normalTiling, 1.0f, 0.1f, 0.1f, 50.0f);
    Bind(prefix + "NormInten", &matData->normalIntensity, 1.0f, 0.01f, 0.0f, 10.0f);

    BindBool(prefix + "OutlineEnable", &matData->enableOutline, false);
    Bind(prefix + "OutlineWidth", &matData->outlineWidth, 1.0f, 0.1f, 0.0f, 50.0f);
    BindColor(prefix + "OutlineColor", &matData->outlineColor, { 0.0f, 0.0f, 0.0f, 1.0f });

    BindBool(prefix + "RippleEnable", &matData->enableRipple, false);

    Bind(prefix + "Wetness", &matData->wetness, 0.5f, 0.01f, 0.0f, 1.0f);

    Bind(prefix + "RippleScale", &matData->rippleScale, 2.0f, 0.1f, 0.1f, 50.0f);
    Bind(prefix + "RippleSpeed", &matData->rippleSpeed, 1.0f, 0.1f, 0.0f, 20.0f);
    Bind(prefix + "RippleStren", &matData->rippleStrength, 0.05f, 0.01f, 0.0f, 5.0f);

    Bind(prefix + "RippleSize", &matData->rippleSize, 0.4f, 0.01f, 0.01f, 5.0f);
    Bind(prefix + "RippleFreq", &matData->rippleFrequency, 1.0f, 0.1f, 0.01f, 10.0f);
    Bind(prefix + "RippleMix", &matData->rippleLayerMix, 0.5f, 0.01f, 0.0f, 1.0f);
}

template <typename ModelType>
bool PropertyBinder::DrawMaterialUI(ModelType* targetModel, const std::string& prefix, GlobalVariables* gv, const std::vector<std::string>& groupPath)
{
#ifdef ENABLE_IMGUI
    if (!targetModel) return false;

    bool isChanged = false; // 変更検知用フラグ
    size_t matCount = targetModel->GetMaterialCount();

    // 一括操作
    if (matCount > 1 && ImGui::TreeNodeEx("BatchOperations", ImGuiTreeNodeFlags_Framed, "一括操作 (全マテリアル適用)"))
    {
        static Vector4 batchColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        ImGui::ColorEdit4("カラー##BatchColor", &batchColor.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview);
        ImGui::SameLine();
        if (ImGui::Button("適用##ApplyBatchColor", ImVec2(60, 0))) {
            targetModel->SetColor(batchColor);
            isChanged = true; // ボタン押下で変更あり
        }

        ImGui::Separator();

        static bool batchOutlineEnable = false;
        static float batchOutlineWidth = 1.0f;
        if (ImGui::Checkbox("アウトライン##BatchOutline", &batchOutlineEnable)) {
            targetModel->SetEnableOutline(batchOutlineEnable);
            isChanged = true;
        }
        ImGui::SameLine(130.0f);
        ImGui::SetNextItemWidth(100.0f);
        if (ImGui::DragFloat("太さ##BatchWidth", &batchOutlineWidth, 0.1f, 0.0f, 10.0f)) {
            targetModel->SetOutlineWidth(batchOutlineWidth);
            isChanged = true;
        }

        ImGui::TreePop();
    }

    ImGui::Spacing();
    ImGui::SeparatorText("マテリアル");

    if (matCount > 0)
    {
        if (ImGui::BeginTabBar("MaterialTabs", ImGuiTabBarFlags_FittingPolicyScroll))
        {
            for (size_t i = 0; i < matCount; ++i)
            {
                std::string matPrefix = (matCount == 1) ? prefix : prefix + "Mat" + std::to_string(i) + "_";
                std::string matTabName = (matCount == 1) ? "マテリアル" : "マテリアル " + std::to_string(i);

                if (ImGui::BeginTabItem(matTabName.c_str()))
                {
                    ImGui::Spacing();

                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "基本 & 質感");
                    isChanged |= Draw(matPrefix + "Color", "カラー");
                    isChanged |= Draw(matPrefix + "Lighting", "ライティング有効");
                    isChanged |= Draw(matPrefix + "LightMode", "照明モード");

                    int currentMode = gv->GetIntValue(groupPath, matPrefix + "LightMode");
                    ImGui::Indent(10.0f);
                    if (currentMode == 3)
                    {
                        isChanged |= Draw(matPrefix + "Roughness", "粗さ");
                        isChanged |= Draw(matPrefix + "Metalness", "金属度");
                    }
                    else
                    {
                        isChanged |= Draw(matPrefix + "Shininess", "光沢度");
                        isChanged |= Draw(matPrefix + "SpecColor", "スペキュラ色");
                        isChanged |= Draw(matPrefix + "DiffuseRef", "拡散反射率");
                    }
                    ImGui::Unindent(10.0f);

                    ImGui::Spacing();
                    isChanged |= Draw(matPrefix + "EnvMapInt", "環境マップ強度");
                    isChanged |= Draw(matPrefix + "Emissive", "自己発光強度");
                    isChanged |= Draw(matPrefix + "AlphaThres", "透過カット閾値(アルファテスト)");

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "テクスチャマップ");
                    isChanged |= Draw(matPrefix + "AlbedoMap", "メインテクスチャ");
                    isChanged |= Draw(matPrefix + "EnvMapTex", "環境マップ");
                    isChanged |= Draw(matPrefix + "ToonRampTex", "トゥーンランプ");

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGuiTreeNodeFlags optFlags = ImGuiTreeNodeFlags_SpanAvailWidth;

                    if (ImGui::TreeNodeEx("ShadowSettings", optFlags, "影設定"))
                    {
                        isChanged |= Draw(matPrefix + "AddShadow", "影を受ける");
                        if (gv->GetIntValue(groupPath_, matPrefix + "AddShadow") > 0)
                        {
                            isChanged |= Draw(matPrefix + "ShadowDens", "影の濃さ");
                            isChanged |= Draw(matPrefix + "ShadowEnv", "環境光の影の強さ");
                            isChanged |= Draw(matPrefix + "ShadowBias", "バイアス");
                            isChanged |= Draw(matPrefix + "ShadowSoft", "柔らかさ");
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    if (ImGui::TreeNodeEx("NormalMapSettings", optFlags, "法線マップ"))
                    {
                        isChanged |= Draw(matPrefix + "NormEnable", "有効化");
                        if (gv->GetIntValue(groupPath_, matPrefix + "NormEnable") > 0)
                        {
                            isChanged |= Draw(matPrefix + "NormalMapTex", "テクスチャ");
                            isChanged |= Draw(matPrefix + "NormInten", "凹凸の強さ");
                            isChanged |= Draw(matPrefix + "NormTile", "タイリング");
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    if (ImGui::TreeNodeEx("RimLightSettings", optFlags, "リムライト"))
                    {
                        isChanged |= Draw(matPrefix + "RimEnable", "有効化");
                        if (gv->GetIntValue(groupPath_, matPrefix + "RimEnable") > 0)
                        {
                            isChanged |= Draw(matPrefix + "RimColor", "発光色");
                            isChanged |= Draw(matPrefix + "RimInten", "強度");
                            isChanged |= Draw(matPrefix + "RimPower", "鋭さ");
                            isChanged |= Draw(matPrefix + "RimUseDir", "ライト方向依存");
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    if (ImGui::TreeNodeEx("EffectSettings", optFlags, "アウトライン / ディゾルブ"))
                    {
                        isChanged |= Draw(matPrefix + "OutlineEnable", "アウトライン有効");
                        if (gv->GetIntValue(groupPath_, matPrefix + "OutlineEnable") > 0)
                        {
                            isChanged |= Draw(matPrefix + "OutlineWidth", "線の太さ");
                            isChanged |= Draw(matPrefix + "OutlineColor", "線の色");
                        }
                        ImGui::Separator();
                        isChanged |= Draw(matPrefix + "DisEnable", "ディゾルブ有効");
                        if (gv->GetIntValue(groupPath_, matPrefix + "DisEnable") > 0)
                        {
                            isChanged |= Draw(matPrefix + "DissolveTex", "ノイズマップ");
                            isChanged |= Draw(matPrefix + "DisThres", "進行度");
                            isChanged |= Draw(matPrefix + "EdgeWidth", "エッジ幅");
                            isChanged |= Draw(matPrefix + "EdgeInten", "エッジ強度");
                            isChanged |= Draw(matPrefix + "EdgeColor", "エッジ色");
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    if (ImGui::TreeNodeEx("WaterSettings", optFlags, "雨の波紋"))
                    {
                        isChanged |= Draw(matPrefix + "RippleEnable", "有効化");
                        if (gv->GetIntValue(groupPath_, matPrefix + "RippleEnable") > 0)
                        {
                            isChanged |= Draw(matPrefix + "Wetness", "濡れ具合 / 水位");

                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "波紋設定");
                            isChanged |= Draw(matPrefix + "RippleMap", "波紋法線マップ");
                            isChanged |= Draw(matPrefix + "RippleScale", "雨の密度(スケール)");
                            isChanged |= Draw(matPrefix + "RippleStren", "波紋の強さ(法線)");
                            isChanged |= Draw(matPrefix + "RippleSpeed", "波紋の全体速度");
                            isChanged |= Draw(matPrefix + "RippleSize", "波紋の広がりサイズ");
                            isChanged |= Draw(matPrefix + "RippleFreq", "波紋の発生頻度");
                            isChanged |= Draw(matPrefix + "RippleMix", "波紋のレイヤー合成率");
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    if (ImGui::TreeNodeEx("UVSettings", optFlags, "UV トランスフォーム"))
                    {
                        Draw(matPrefix + "UVTrans", "UV 位置");
                        Draw(matPrefix + "UVRot", "UV 回転");
                        Draw(matPrefix + "UVScale", "UV スケール");
                        
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }

    return isChanged;
#else
    return false;
#endif
}

}