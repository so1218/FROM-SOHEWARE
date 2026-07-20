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
    // モデル情報をマップに保存（拡張用）
    modelBindMap_[groupName] = { model };

    auto* mat = model->GetMaterialData();
    auto* transform = &model->GetTransform();
    auto* uvTransform = model->GetUVTransform();

    std::string prefix = groupName + "_";

    Bind(prefix + "Trans", &transform->translation_, { 0.0f, 0.0f, 0.0f }, 0.1f);
    BindRotation(prefix + "Rot", &transform->rotation_, &transform->rotationQuaternion_, 0.01f);
    Bind(prefix + "Scale", &transform->scale_, { 1.0f, 1.0f, 1.0f }, 0.1f);

    size_t matCount = model->GetMaterialCount();
    for (size_t i = 0; i < matCount; ++i)
    {
        std::string matPrefix;

        if (matCount == 1)
        {
            // マテリアルが1つならMat0を省略
            matPrefix = prefix;
        }
        else
        {
            // 複数あるならMatを付ける
            matPrefix = prefix + "Mat" + std::to_string(i) + "_";
        }

        // ヘルパー関数を呼び出す
        BindMaterialProperties(matPrefix, model->GetMaterialHandle(i));
    }
}

void PropertyBinder::DrawModel(const std::string& groupName, const std::string& customLabel)
{
#ifdef IS_DEVELOPMENT
    std::string prefix = groupName + "_";
    auto* gv = GlobalVariables::GetInstance();
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    Model* targetModel = nullptr;
    if (modelBindMap_.find(groupName) != modelBindMap_.end()) {
        targetModel = modelBindMap_[groupName].model;
    }

    ImGui::PushID(groupName.c_str());
    if (ImGui::CollapsingHeader(displayLabel.c_str()))
    {
        ImGui::Spacing();
        ImGui::SeparatorText("トランスフォーム");
        Draw(prefix + "Trans", "位置");
        Draw(prefix + "Rot", "回転");
        Draw(prefix + "Scale", "スケール");

        ImGui::Spacing();

        // 共通関数を呼び出す
        DrawMaterialUI(targetModel, prefix, gv, groupPath_);
    }
    ImGui::PopID();
#endif
}

void PropertyBinder::BindAnimationModel(const std::string& groupName, AnimationModel* model)
{
    AnimationBindInfo info;
    info.model = model;
    animationBindMap_[groupName] = info;

    auto* mat = model->GetMaterialData();
    auto* transform = &model->GetTransform();

    std::string prefix = groupName + "_";

    Bind(prefix + "Trans", &transform->translation_, { 0.0f, 0.0f, 0.0f }, 0.1f);
    BindRotation(prefix + "Rot", &transform->rotation_, &transform->rotationQuaternion_, 0.01f);
    Bind(prefix + "Scale", &transform->scale_, { 1.0f, 1.0f, 1.0f }, 0.1f);

    size_t matCount = model->GetMaterialCount();
    for (size_t i = 0; i < matCount; ++i)
    {
        std::string matPrefix;

        if (matCount == 1)
        {
            // マテリアルが1つならMat0を省略
            matPrefix = prefix;
        }
        else
        {
            // 複数あるなら Matを付ける
            matPrefix = prefix + "Mat" + std::to_string(i) + "_";
        }

        // ヘルパー関数を呼び出す
        BindMaterialProperties(matPrefix, model->GetMaterialHandle(i));
    }

    Bind(prefix + "SpeedScale", model->GetSpeedScalePtr(), 1.0f, 0.1f, 0.0f, 5.0f);
    Bind(prefix + "IsLoop", model->GetIsLoopPtr(), true);
}

void PropertyBinder::DrawAnimationModel(const std::string& groupName, const std::string& customLabel)
{
#ifdef IS_DEVELOPMENT
    std::string prefix = groupName + "_";
    auto* gv = GlobalVariables::GetInstance();
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    AnimationModel* targetModel = nullptr;
    if (animationBindMap_.find(groupName) != animationBindMap_.end()) {
        targetModel = animationBindMap_[groupName].model;
    }

    ImGui::PushID(groupName.c_str());
    if (ImGui::CollapsingHeader(displayLabel.c_str()))
    {
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

        // 共通関数を呼び出す
        DrawMaterialUI(targetModel, prefix, gv, groupPath_);
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
        TextureType::Albedo             // フィルタ
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
#ifdef IS_DEVELOPMENT
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
    auto& texManager = TextureManager::GetInstance();
    GlobalVariables* gv = GlobalVariables::GetInstance();

    // デフォルト値の決定と登録
    std::string valToSave = initialValue.empty() ? defaultName : initialValue;

    // GlobalVariablesに項目を追加
    gv->AddItem(groupPath_, key, valToSave);

    // 初期同期処理
    std::string savedValue = gv->GetStringValue(groupPath_, key);

    // オブジェクトが持っている値と保存されていた値が違う場合
    if (savedValue != initialValue)
    {
        if (onValueChanged)
        {
            onValueChanged(savedValue); // SpriteやMaterialの値を更新
        }
    }


    // 描画処理の登録 (ラムダ式)
    items_[key] = [this, key, filterType, defaultName, onValueChanged](const std::string& label)
        {
            auto& texManager = TextureManager::GetInstance();
            auto* srvManager = this->engine_->GetSRVManager();
            GlobalVariables* gv = GlobalVariables::GetInstance();

            // 現在の値をGlobalVariablesから取得
            std::string currentTextureName = gv->GetStringValue(groupPath_, key);

            // ハンドルは名前からその場で引く
            uint32_t currentHandle = texManager.Get(currentTextureName);
            auto currentGpuHandle = srvManager->GetSRVHandleGPU(currentHandle);

            // メタデータを取得
            const TextureHandleData* currentMeta = texManager.GetMetaData(currentTextureName);
            bool isCurrentCubeMap = (currentMeta && currentMeta->type == TextureType::CubeMap);

#ifdef IS_DEVELOPMENT
            // ラベル表示
            std::string displayLabel = label.empty() ? key : label;
            ImGui::Text("%s", displayLabel.c_str());

            std::string popupId = "Popup_" + key;
            bool openPopup = false;

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

            // プレビューボタン表示
            ImVec2 previewSize(64, 64);

            if (isCurrentCubeMap)
            {
                if (ImGui::Button("CUBE\nMAP", previewSize)) { openPopup = true; }
            }
            else
            {
                if (currentGpuHandle.ptr != 0)
                {
                    if (ImGui::ImageButton(key.c_str(), (ImTextureID)currentGpuHandle.ptr, previewSize,
                        ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
                    {
                        openPopup = true;
                    }
                }
                else
                {
                    if (ImGui::Button("Null", previewSize)) { openPopup = true; }
                }
            }

            ImGui::PopStyleColor();

            // ツールチップ
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Name: %s", currentTextureName.c_str());
                if (!isCurrentCubeMap && currentGpuHandle.ptr != 0) {
                    ImGui::Image((ImTextureID)currentGpuHandle.ptr, ImVec2(128, 128));
                }
                ImGui::EndTooltip();
            }

            ImGui::SameLine();

            // テキストボックス編集
            char buffer[256];
            strncpy_s(buffer, currentTextureName.c_str(), _TRUNCATE);

            bool valueChanged = false;
            std::string newName = currentTextureName;

            if (ImGui::InputText(("##TexName_" + key).c_str(), buffer, sizeof(buffer)))
            {
                newName = std::string(buffer);
                valueChanged = true;
            }

            // ポップアップ（選択パレット）
            if (openPopup) {
                ImGui::OpenPopup(popupId.c_str());
            }

            ImGui::SetNextWindowSizeConstraints(ImVec2(300, 200), ImVec2(800, 600));

            if (ImGui::BeginPopup(popupId.c_str()))
            {
                const auto& allTextures = texManager.GetAllTextures();
                ImGuiStyle& style = ImGui::GetStyle();
                ImVec2 buttonSize(48.0f, 48.0f);
                float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                int displayedCount = 0;

                for (size_t i = 0; i < allTextures.size(); i++)
                {
                    const auto& data = allTextures[i];
                    if (data.type != filterType) { continue; }

                    auto hGPU = srvManager->GetSRVHandleGPU(data.handle);
                    ImGui::PushID((int)i);

                    bool isSelected = (currentTextureName == data.name);
                    if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                    else            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

                    bool clicked = false;
                    bool isItemCubeMap = (data.type == TextureType::CubeMap);

                    if (isItemCubeMap) {
                        if (ImGui::Button("CUBE", buttonSize)) clicked = true;
                    }
                    else {
                        if (ImGui::ImageButton("Tex", (ImTextureID)hGPU.ptr, buttonSize)) clicked = true;
                    }

                    ImGui::PopStyleColor();

                    if (clicked) {
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
                        if (!isItemCubeMap && hGPU.ptr != 0) {
                            ImGui::Image((ImTextureID)hGPU.ptr, ImVec2(128.0f, 128.0f));
                        }
                        ImGui::EndTooltip();
                    }

                    ImGui::PopID();

                    float lastButtonX2 = ImGui::GetItemRectMax().x;
                    float nextButtonX2 = lastButtonX2 + style.ItemSpacing.x + buttonSize.x;
                    if (nextButtonX2 < windowVisibleX2) { ImGui::SameLine(); }
                    displayedCount++;
                }
                if (displayedCount == 0) { ImGui::TextDisabled("No textures found."); }
                ImGui::EndPopup();
            }

            // 値の更新処理
            // 変更があった場合のみ保存＆コールバック実行
            if (valueChanged)
            {
                // GlobalVariablesに保存
                gv->SetValue(groupPath_, key, newName);

                // Sprite側へ通知
                if (onValueChanged)
                {
                    onValueChanged(newName);
                }
            }
#endif
        };
}

void PropertyBinder::BindTexture(
    const std::string& key,
    std::string* currentNamePtr,
    uint32_t* currentHandlePtr,
    const std::string& defaultName,
    TextureType filterType,
    std::function<void()> callback) 
{
    // ポインタがnullなら何もしない（安全対策）
    if (!currentNamePtr || !currentHandlePtr) return;

    // 自動的にラムダ式を作成して内部のBindTextureへ渡す
    BindTexture(
        key,              // キー
        *currentNamePtr,  // 現在の値

        // callbackをキャプチャ [=] や [..., callback] で取り込む
        [currentNamePtr, currentHandlePtr, callback](const std::string& newName)
        {
            // ポインタ先の変数を更新
            *currentNamePtr = newName;

            // ハンドル更新
            *currentHandlePtr = TextureManager::GetInstance().Get(newName);

            // テクスチャが変更されたら、登録されたコールバックを実行
            if (callback)
            {
                callback();
            }
        },

        defaultName,      // デフォルト値
        filterType        // フィルタ
    );
}

void PropertyBinder::BindTerrain(const std::string& groupName, Terrain* terrain)
{
    if (!terrain) return;
    terrainBindMap_[groupName] = { terrain };

    std::string prefix = groupName + "_";

    Bind(prefix + "Position", &terrain->GetTransform().translation_, Vector3(0, 0, 0), 1.0f);

    // 地形形状・パラメータ変化用のコールバック
    auto onTerrainShapeChanged = [terrain]() {
        terrain->RebuildMesh();
        };

    // ハイトマップがインスペクターで変更されたときのコールバック
    auto onHeightmapChanged = [terrain]() {
        // 現在の設定値を維持したまま、新しいハイトマップ画像からメッシュを再生成
        terrain->LoadFromHeightmap(
            terrain->GetHeightmapName(),
            terrain->GetParams().cellSize
        );
        };

    BindTexture(prefix + "Heightmap", terrain->GetHeightmapNamePtr(), terrain->GetHeightmapHandlePtr(), "white1x1", TextureType::Noise, onHeightmapChanged);

    Bind(prefix + "MaxHeight", &terrain->GetParams().maxHeight, 20.0f, 0.1f, 0.0f, 500.0f, onTerrainShapeChanged);

    BindMaterialProperties(prefix, terrain->GetMaterialHandle());
}

void PropertyBinder::DrawTerrain(const std::string& groupName, const std::string& customLabel)
{
#ifdef IS_DEVELOPMENT
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
    BindTexture(prefix + "EnvMapTex", &handle->envMapName, &handle->envMapHandle, "skybox", TextureType::CubeMap);
    BindTexture(prefix + "NormalMapTex", &handle->normalMapName, &handle->normalMapHandle, "normal_01", TextureType::Normal);
    BindTexture(prefix + "DissolveTex", &handle->dissolveMapName, &handle->dissolveMapHandle, "white1x1", TextureType::Noise);
    BindTexture(prefix + "ToonRampTex", &handle->toonRampName, &handle->toonRampHandle, "toonRamp_01", TextureType::Toon);
    BindTexture(prefix + "RippleMap", &handle->rippleTextureName, &handle->rippleTextureHandle, "normal_00", TextureType::Normal);
    BindTexture(prefix + "PuddleNoise", &handle->puddleNoiseName, &handle->puddleNoiseHandle, "noise_00", TextureType::Noise);

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

    BindBool(prefix + "UseTriplanar", &matData->useTriplanar, false);
    Bind(prefix + "TriScale", &matData->triplanarScale, 0.1f, 0.005f, 0.001f, 10.0f);
    Bind(prefix + "TriSharpness", &matData->triplanarBlendSharpness, 4.0f, 0.1f, 1.0f, 16.0f);

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
    BindBool(prefix + "UsePuddle", &matData->usePuddle, false);

    Bind(prefix + "Wetness", &matData->wetness, 0.5f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "PuddleEmission", &matData->puddleEmission, 0.0f, 0.1f, 0.0f, 50.0f);

    Bind(prefix + "RippleScale", &matData->rippleScale, 2.0f, 0.1f, 0.1f, 50.0f);
    Bind(prefix + "RippleSpeed", &matData->rippleSpeed, 1.0f, 0.1f, 0.0f, 20.0f);
    Bind(prefix + "RippleStren", &matData->rippleStrength, 0.05f, 0.01f, 0.0f, 5.0f);

    Bind(prefix + "PuddleScale", &matData->puddleScale, 0.1f, 0.01f, 0.001f, 10.0f);
    Bind(prefix + "PuddleFalloff", &matData->puddleFalloff, 0.1f, 0.005f, 0.001f, 0.5f);
    Bind(prefix + "RippleSize", &matData->rippleSize, 0.4f, 0.01f, 0.01f, 5.0f);
    Bind(prefix + "RippleFreq", &matData->rippleFrequency, 1.0f, 0.1f, 0.01f, 10.0f);
    Bind(prefix + "RippleMix", &matData->rippleLayerMix, 0.5f, 0.01f, 0.0f, 1.0f);

    BindColor(prefix + "PuddleColor", &matData->puddleColor, { 0.1f, 0.1f, 0.1f, 0.5f });
    Bind(prefix + "PuddleTint", &matData->puddleTint, 0.5f, 0.01f, 0.0f, 1.0f);
}

template <typename ModelType>
void PropertyBinder::DrawMaterialUI(ModelType* targetModel, const std::string& prefix, GlobalVariables* gv, const std::vector<std::string>& groupPath)
{
#ifdef IS_DEVELOPMENT
    if (!targetModel) return;

    size_t matCount = targetModel->GetMaterialCount();

    // 一括操作
    if (matCount > 1 && ImGui::TreeNodeEx("BatchOperations", ImGuiTreeNodeFlags_Framed, "一括操作 (全マテリアル適用)"))
    {
        static Vector4 batchColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        ImGui::ColorEdit4("カラー##BatchColor", &batchColor.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview);
        ImGui::SameLine();
        if (ImGui::Button("適用##ApplyBatchColor", ImVec2(60, 0))) { targetModel->SetColor(batchColor); }

        ImGui::Separator();

        static bool batchOutlineEnable = false;
        static float batchOutlineWidth = 1.0f;
        if (ImGui::Checkbox("アウトライン##BatchOutline", &batchOutlineEnable)) { targetModel->SetEnableOutline(batchOutlineEnable); }
        ImGui::SameLine(130.0f);
        ImGui::SetNextItemWidth(100.0f);
        if (ImGui::DragFloat("太さ##BatchWidth", &batchOutlineWidth, 0.1f, 0.0f, 10.0f)) { targetModel->SetOutlineWidth(batchOutlineWidth); }

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
                    Draw(matPrefix + "Color", "カラー");
                    Draw(matPrefix + "Lighting", "ライティング有効");
                    Draw(matPrefix + "LightMode", "照明モード");

                    int currentMode = gv->GetIntValue(groupPath, matPrefix + "LightMode");
                    ImGui::Indent(10.0f);
                    if (currentMode == 3)
                    {
                        Draw(matPrefix + "Roughness", "粗さ");
                        Draw(matPrefix + "Metalness", "金属度");
                    }
                    else
                    {
                        Draw(matPrefix + "Shininess", "光沢度");
                        Draw(matPrefix + "SpecColor", "スペキュラ色");
                        Draw(matPrefix + "DiffuseRef", "拡散反射率");
                    }
                    ImGui::Unindent(10.0f);

                    ImGui::Spacing();
                    Draw(matPrefix + "EnvMapInt", "環境マップ強度");
                    Draw(matPrefix + "Emissive", "自己発光強度");
                    Draw(matPrefix + "AlphaThres", "透過カット閾値(アルファテスト)");

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "テクスチャマップ");
                    Draw(matPrefix + "AlbedoMap", "メインテクスチャ");
                    Draw(matPrefix + "EnvMapTex", "環境マップ");
                    Draw(matPrefix + "ToonRampTex", "トゥーンランプ");

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGuiTreeNodeFlags optFlags = ImGuiTreeNodeFlags_Framed;

                    if (ImGui::TreeNodeEx("ShadowSettings", optFlags, "影設定"))
                    {
                        Draw(matPrefix + "AddShadow", "影を受ける");
                        if (gv->GetIntValue(groupPath_, matPrefix + "AddShadow") > 0)
                        {
                            Draw(matPrefix + "ShadowDens", "影の濃さ(不透明度)");
                            Draw(matPrefix + "ShadowEnv", "環境光の影の強さ");
                            Draw(matPrefix + "ShadowBias", "バイアス");
                            Draw(matPrefix + "ShadowSoft", "柔らかさ");
                        }
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNodeEx("NormalMapSettings", optFlags, "法線マップ"))
                    {
                        Draw(matPrefix + "NormEnable", "有効化");
                        if (gv->GetIntValue(groupPath_, matPrefix + "NormEnable") > 0)
                        {
                            Draw(matPrefix + "NormalMapTex", "テクスチャ");
                            Draw(matPrefix + "NormInten", "凹凸の強さ");
                            Draw(matPrefix + "NormTile", "タイリング");
                        }
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNodeEx("RimLightSettings", optFlags, "リムライト"))
                    {
                        Draw(matPrefix + "RimEnable", "有効化");
                        if (gv->GetIntValue(groupPath_, matPrefix + "RimEnable") > 0)
                        {
                            Draw(matPrefix + "RimColor", "発光色");
                            Draw(matPrefix + "RimInten", "強度");
                            Draw(matPrefix + "RimPower", "鋭さ");
                            Draw(matPrefix + "RimUseDir", "ライト方向依存");
                        }
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNodeEx("EffectSettings", optFlags, "特殊エフェクト (アウトライン / ディゾルブ)"))
                    {
                        Draw(matPrefix + "OutlineEnable", "アウトライン有効");
                        if (gv->GetIntValue(groupPath_, matPrefix + "OutlineEnable") > 0)
                        {
                            Draw(matPrefix + "OutlineWidth", "線の太さ");
                            Draw(matPrefix + "OutlineColor", "線の色");
                        }
                        ImGui::Separator();
                        Draw(matPrefix + "DisEnable", "ディゾルブ有効");
                        if (gv->GetIntValue(groupPath_, matPrefix + "DisEnable") > 0)
                        {
                            Draw(matPrefix + "DissolveTex", "ノイズマップ");
                            Draw(matPrefix + "DisThres", "進行度");
                            Draw(matPrefix + "EdgeWidth", "エッジ幅");
                            Draw(matPrefix + "EdgeInten", "エッジ強度");
                            Draw(matPrefix + "EdgeColor", "エッジ色");
                        }
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNodeEx("WaterSettings", optFlags, "水エフェクト (水たまり / 波紋)"))
                    {
                        Draw(matPrefix + "RippleEnable", "有効化");
                        if (gv->GetIntValue(groupPath_, matPrefix + "RippleEnable") > 0)
                        {
                            Draw(matPrefix + "Wetness", "濡れ具合 / 水位");

                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "波紋 (Ripple) 設定");
                            Draw(matPrefix + "RippleMap", "波紋法線マップ");
                            Draw(matPrefix + "RippleScale", "雨の密度(スケール)");
                            Draw(matPrefix + "RippleStren", "波紋の強さ(法線)");
                            Draw(matPrefix + "RippleSpeed", "波紋の全体速度");
                            Draw(matPrefix + "RippleSize", "波紋の広がりサイズ");
                            Draw(matPrefix + "RippleFreq", "波紋の発生頻度");
                            Draw(matPrefix + "RippleMix", "波紋のレイヤー合成率");

                            ImGui::Spacing();
                            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "水たまり (Puddle) 設定");
                            Draw(matPrefix + "UsePuddle", "水たまり形成");
                            if (gv->GetIntValue(groupPath_, matPrefix + "UsePuddle") > 0)
                            {
                                Draw(matPrefix + "PuddleNoise", "分布ノイズマップ");
                                Draw(matPrefix + "PuddleScale", "ノイズスケール");
                                Draw(matPrefix + "PuddleFalloff", "エッジの滑らかさ");
                                Draw(matPrefix + "PuddleColor", "水の色と濁り(Alpha)");
                                Draw(matPrefix + "PuddleTint", "水の色合い調整");
                                Draw(matPrefix + "PuddleEmission", "水たまりの発光強度");
                            }
                        }
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNodeEx("UVSettings", optFlags, "UV トランスフォーム"))
                    {
                        Draw(matPrefix + "UseTriplanar", "トライプラナー有効");
                        if (gv->GetIntValue(groupPath_, matPrefix + "UseTriplanar") > 0)
                        {
                            Draw(matPrefix + "TriScale", "テクスチャスケール");
                            Draw(matPrefix + "TriSharpness", "ブレンドのシャープさ");
                        }
                        else
                        {
                            Draw(matPrefix + "UVTrans", "UV 位置");
                            Draw(matPrefix + "UVRot", "UV 回転");
                            Draw(matPrefix + "UVScale", "UV スケール");
                        }
                        ImGui::TreePop();
                    }

                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }
#endif 
}

}