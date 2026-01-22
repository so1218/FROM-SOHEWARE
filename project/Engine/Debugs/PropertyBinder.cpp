#include "PropertyBinder.h"
#include "Model.h"
#include "AnimationModel.h"
#include "Sprite.h"

void PropertyBinder::BindModel(const std::string& groupName, Model* model)
{
    // モデル情報をマップに保存（拡張用）
    modelBindMap_[groupName] = { model };

    auto* mat = model->GetMaterial();
    auto* transform = &model->GetTransform();
    auto* uvTransform = &model->GetUVTransform();

    std::string prefix = groupName + "_";

    BindTexture(prefix + "AlbedoMap", model->GetTextureHandlePtr());
    BindTexture(prefix + "EnvMapTex", model->GetEnvMapTextureHandlePtr());
    BindTexture(prefix + "ToonRampTex", model->GetToonRampHandlePtr());
    BindTexture(prefix + "NormalMapTex", model->GetNormalMapHandlePtr());
    BindTexture(prefix + "DissolveTex", model->GetDissolveTextureHandlePtr());

    Bind(prefix + "Trans", &transform->translation_, { 0.0f, 0.0f, 0.0f }, 0.1f);
    BindRotation(prefix + "Rot", &transform->rotation_, &transform->rotationQuaternion_, 0.01f);
    Bind(prefix + "Scale", &transform->scale_, { 1.0f, 1.0f, 1.0f }, 0.1f);

    auto onUVChange = [model]()
        {
            model->UpdateUV();
        };

    Bind(prefix + "UVTrans", &uvTransform->translation_, { 0.0f, 0.0f, 0.0f }, 0.01f, onUVChange);
    BindRotation(prefix + "UVRot", &uvTransform->rotation_, &uvTransform->rotationQuaternion_, 0.01f, onUVChange);
    Bind(prefix + "UVScale", &uvTransform->scale_, { 1.0f, 1.0f, 1.0f }, 0.01f, onUVChange);

    BindColor(prefix + "Color", model->GetColorPtr(), 0xFFFFFFFF);
    BindBool(prefix + "Lighting", &mat->enableLighting, true);
    BindCombo(prefix + "LightMode", &mat->lightMode, 1, "ハーフランバート\0スペキュラ\0トゥーン\0PBR\0");
    Bind(prefix + "DiffuseRef", &mat->diffuseReflection, 4.0f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "Shininess", &mat->shininess, 50.0f, 0.1f, 1.0f, 256.0f);
    Bind(prefix + "EnvMapInt", &mat->environmentMapIntensity, 0.0f, 0.01f, 0.0f, 10.0f);

    Bind(prefix + "Roughness", &mat->roughness, 0.5f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "Metalness", &mat->metalness, 0.0f, 0.01f, 0.0f, 1.0f);
    BindColor(prefix + "SpecColor", &mat->specularColor, { 1.0f, 1.0f, 1.0f, 1.0f });
    Bind(prefix + "Emissive", &mat->emissiveIntensity, 1.0f, 0.1f, 0.0f, 100.0f);

    BindBool(prefix + "AddShadow", &mat->addShadow, true);
    Bind(prefix + "ShadowBias", &mat->shadowBias, 0.0005f, 0.0001f, 0.0f, 0.1f);
    Bind(prefix + "ShadowDens", &mat->shadowDensity, 0.7f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "ShadowSoft", &mat->shadowSoftness, 0.0f, 0.01f, 0.0f, 5.0f);

    BindBool(prefix + "RimEnable", &mat->enableRim, false);
    BindBool(prefix + "RimUseDir", &mat->rimUseLightDir, false);
    Bind(prefix + "RimPower", &mat->rimPower, 3.0f, 0.1f, 0.0f, 20.0f);
    Bind(prefix + "RimInten", &mat->rimIntensity, 1.0f, 0.1f, 0.0f, 10.0f);
    BindColor(prefix + "RimColor", &mat->rimColor, { 1.0f, 1.0f, 1.0f });

    BindBool(prefix + "DisEnable", &mat->enableDissolve, false);
    Bind(prefix + "DisThres", &mat->dissolveThreshold, 0.5f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "EdgeWidth", &mat->edgeWidth, 0.05f, 0.001f, 0.0f, 0.5f);
    Bind(prefix + "EdgeInten", &mat->edgeIntensity, 2.0f, 0.1f, 0.0f, 10.0f);
    BindColor(prefix + "EdgeColor", &mat->edgeColor, { 1.0f, 0.5f, 0.0f });

    BindBool(prefix + "NormEnable", &mat->enableNormalMap, false);
    Bind(prefix + "NormTile", &mat->normalTiling, 1.0f, 0.1f, 0.1f, 50.0f);
    Bind(prefix + "NormInten", &mat->normalIntensity, 1.0f, 0.01f, 0.0f, 10.0f);

    BindBool(prefix + "OutlineEnable", model->GetEnableOutlinePtr(), false);
    Bind(prefix + "OutlineWidth", model->GetOutlineWidthPtr(), 1.0f, 0.1f, 0.0f, 50.0f);
    BindColor(prefix + "OutlineColor", model->GetOutlineColorPtr(), { 0.0f, 0.0f, 0.0f, 1.0f });
}

void PropertyBinder::DrawModel(const std::string& groupName, const std::string& customLabel)
{
#ifdef IS_DEVELOPMENT
    std::string prefix = groupName + "_";
    auto* gv = GlobalVariables::GetInstance();
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    ImGui::PushID(groupName.c_str());

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.4f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.5f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.6f, 0.4f, 1.0f));

    bool isOpened = ImGui::CollapsingHeader(displayLabel.c_str(), ImGuiTreeNodeFlags_None);

    ImGui::PopStyleColor(3);

    if (isOpened)
    {
        ImGui::Indent(20.0f);

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);

        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_FramePadding |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (ImGui::TreeNodeEx("Transform", nodeFlags, "トランスフォーム"))
        {
            ImGui::Spacing();
            Draw(prefix + "Trans", "位置");
            Draw(prefix + "Rot", "回転");
            Draw(prefix + "Scale", "スケール");

            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("UVTransform", nodeFlags, "UV トランスフォーム"))
        {
            ImGui::Spacing();
            Draw(prefix + "UVTrans", "UV 位置");
            Draw(prefix + "UVRot", "UV 回転");
            Draw(prefix + "UVScale", "UV スケール");
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("BasicSettings", nodeFlags, "基本マテリアル設定"))
        {
            ImGui::Spacing();
            ImGui::TextDisabled("パラメーター");
            Draw(prefix + "Color", "カラー");
            Draw(prefix + "Lighting", "ライティング有効");
            Draw(prefix + "LightMode", "照明モード");
            Draw(prefix + "EnvMapInt", "環境マップ強度");
            Draw(prefix + "Emissive", "自己発光強度");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextDisabled("テクスチャ");

            Draw(prefix + "AlbedoMap", "メインテクスチャ");
            Draw(prefix + "EnvMapTex", "環境マップ");
            Draw(prefix + "ToonRampTex", "トゥーンランプ");

            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Surface", nodeFlags, "質感"))
        {
            ImGui::Spacing();

            int currentMode = gv->GetIntValue(groupPath_, prefix + "LightMode");

            bool isPBR = (currentMode == 3);

            if (isPBR)
            {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[ PBR設定 ]");

                Draw(prefix + "Roughness", "粗さ (Roughness)");
                Draw(prefix + "Metalness", "金属度 (Metalness)");
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "[ スタンダード設定 ]");

                Draw(prefix + "Shininess", "光沢度 (Shininess)");
                Draw(prefix + "SpecColor", "スペキュラ色");
                Draw(prefix + "DiffuseRef", "拡散反射率");
            }

            ImGui::Spacing();
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Shadow", nodeFlags, "影設定"))
        {
            ImGui::Spacing();
            Draw(prefix + "AddShadow", "影を受ける (有効化)");

            if (gv->GetIntValue(groupPath_, prefix + "AddShadow") > 0)
            {
                ImGui::Indent(10.0f);

                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "詳細設定:");
                Draw(prefix + "ShadowDens", "影の濃さ");
                Draw(prefix + "ShadowBias", "バイアス調整");
                Draw(prefix + "ShadowSoft", "エッジの柔らかさ");

                ImGui::Unindent(10.0f);
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("RimLight", nodeFlags, "リムライト"))
        {
            ImGui::Spacing();
            Draw(prefix + "RimEnable", "リムライト有効");

            if (gv->GetIntValue(groupPath_, prefix + "RimEnable") > 0)
            {
                ImGui::Indent(10.0f);

                Draw(prefix + "RimColor", "発光色");
                Draw(prefix + "RimInten", "発光強度");
                Draw(prefix + "RimPower", "リムの鋭さ");
                Draw(prefix + "RimUseDir", "ライト方向依存");

                ImGui::Unindent(10.0f);
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("NormalMap", nodeFlags, "法線マップ"))
        {
            ImGui::Spacing();
            Draw(prefix + "NormEnable", "法線マップ有効");

            if (gv->GetIntValue(groupPath_, prefix + "NormEnable") > 0)
            {
                ImGui::Indent(10.0f);

                Draw(prefix + "NormalMapTex", "法線テクスチャ");
                Draw(prefix + "NormInten", "凹凸の強さ");
                Draw(prefix + "NormTile", "タイリング回数");

                ImGui::Unindent(10.0f);
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Dissolve", nodeFlags, "ディゾルブ"))
        {
            ImGui::Spacing();
            Draw(prefix + "DisEnable", "ディゾルブ有効");

            if (gv->GetIntValue(groupPath_, prefix + "DisEnable") > 0)
            {
                ImGui::Indent(10.0f);

                Draw(prefix + "DissolveTex", "ノイズマップ");
                Draw(prefix + "DisThres", "進行度");

                ImGui::Separator();
                ImGui::TextDisabled("Edge Settings");

                Draw(prefix + "EdgeWidth", "エッジ幅");
                Draw(prefix + "EdgeInten", "エッジ発光強度");
                Draw(prefix + "EdgeColor", "エッジ色");

                ImGui::Unindent(10.0f);
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Outline", nodeFlags, "アウトライン"))
        {
            ImGui::Spacing();
            Draw(prefix + "OutlineEnable", "アウトライン有効");

            if (gv->GetBoolValue(groupPath_, prefix + "OutlineEnable"))
            {
                ImGui::Indent(10.0f);

                Draw(prefix + "OutlineWidth", "線の太さ");
                Draw(prefix + "OutlineColor", "線の色");

                ImGui::Unindent(10.0f);
            }
            ImGui::Spacing();
            ImGui::TreePop();
        }

        ImGui::PopItemWidth();

        ImGui::Unindent(20.0f);
    }

    ImGui::PopID();
#endif
}

void PropertyBinder::BindAnimationModel(const std::string& groupName, AnimationModel* model)
{
    auto* mat = model->GetMaterialData();
    auto* transform = model->GetTransformPtr();
    auto* uvTransform = model->GetUVTransformPtr();

    std::string prefix = groupName + "_";

    BindTexture(prefix + "AlbedoMap", model->GetTextureHandlePtr());
    BindTexture(prefix + "EnvMapTex", model->GetEnvMapTextureHandlePtr());
    BindTexture(prefix + "ToonRampTex", model->GetToonRampHandlePtr());
    BindTexture(prefix + "NormalMapTex", model->GetNormalMapHandlePtr());
    BindTexture(prefix + "DissolveTex", model->GetDissolveTextureHandlePtr());

    Bind(prefix + "Trans", &transform->translation_, { 0.0f, 0.0f, 0.0f }, 0.1f);
    BindRotation(prefix + "Rot", &transform->rotation_, &transform->rotationQuaternion_, 0.01f);
    Bind(prefix + "Scale", &transform->scale_, { 1.0f, 1.0f, 1.0f }, 0.1f);

    auto onUVChange = [model]()
        {
            model->UpdateUV();
        };

    Bind(prefix + "UVTrans", &uvTransform->translation_, { 0.0f, 0.0f, 0.0f }, 0.01f, onUVChange);
    BindRotation(prefix + "UVRot", &uvTransform->rotation_, &uvTransform->rotationQuaternion_, 0.01f, onUVChange);
    Bind(prefix + "UVScale", &uvTransform->scale_, { 1.0f, 1.0f, 1.0f }, 0.01f, onUVChange);

    BindColor(prefix + "Color", model->GetColorPtr(), 0xFFFFFFFF);
    BindBool(prefix + "Lighting", &mat->enableLighting, true);
    BindCombo(prefix + "LightMode", &mat->lightMode, 1, "ハーフランバート\0スペキュラ\0トゥーン\0PBR\0");
    Bind(prefix + "DiffuseRef", &mat->diffuseReflection, 4.0f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "Shininess", &mat->shininess, 50.0f, 0.1f, 1.0f, 256.0f);
    Bind(prefix + "EnvMapInt", &mat->environmentMapIntensity, 0.0f, 0.01f, 0.0f, 10.0f);

    Bind(prefix + "Roughness", &mat->roughness, 0.5f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "Metalness", &mat->metalness, 0.0f, 0.01f, 0.0f, 1.0f);
    BindColor(prefix + "SpecColor", &mat->specularColor, { 1.0f, 1.0f, 1.0f, 1.0f });
    Bind(prefix + "Emissive", &mat->emissiveIntensity, 1.0f, 0.1f, 0.0f, 100.0f);

    BindBool(prefix + "AddShadow", &mat->addShadow, true);
    Bind(prefix + "ShadowBias", &mat->shadowBias, 0.0005f, 0.0001f, 0.0f, 0.1f);
    Bind(prefix + "ShadowDens", &mat->shadowDensity, 0.7f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "ShadowSoft", &mat->shadowSoftness, 0.0f, 0.01f, 0.0f, 5.0f);

    BindBool(prefix + "RimEnable", &mat->enableRim, false);
    BindBool(prefix + "RimUseDir", &mat->rimUseLightDir, false);
    Bind(prefix + "RimPower", &mat->rimPower, 3.0f, 0.1f, 0.0f, 20.0f);
    Bind(prefix + "RimInten", &mat->rimIntensity, 1.0f, 0.1f, 0.0f, 10.0f);
    BindColor(prefix + "RimColor", &mat->rimColor, { 1.0f, 1.0f, 1.0f });

    BindBool(prefix + "DisEnable", &mat->enableDissolve, false);
    Bind(prefix + "DisThres", &mat->dissolveThreshold, 0.5f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "EdgeWidth", &mat->edgeWidth, 0.05f, 0.001f, 0.0f, 0.5f);
    Bind(prefix + "EdgeInten", &mat->edgeIntensity, 2.0f, 0.1f, 0.0f, 10.0f);
    BindColor(prefix + "EdgeColor", &mat->edgeColor, { 1.0f, 0.5f, 0.0f });

    BindBool(prefix + "NormEnable", &mat->enableNormalMap, false);
    Bind(prefix + "NormTile", &mat->normalTiling, 1.0f, 0.1f, 0.1f, 50.0f);
    Bind(prefix + "NormInten", &mat->normalIntensity, 1.0f, 0.01f, 0.0f, 10.0f);

    BindBool(prefix + "OutlineEnable", model->GetEnableOutlinePtr(), false);
    Bind(prefix + "OutlineWidth", model->GetOutlineWidthPtr(), 1.0f, 0.1f, 0.0f, 50.0f);
    BindColor(prefix + "OutlineColor", model->GetOutlineColorPtr(), { 0.0f, 0.0f, 0.0f, 1.0f });

    Bind(prefix + "SpeedScale", model->GetSpeedScalePtr(), 1.0f, 0.1f, 0.0f, 5.0f);
    BindBool(prefix + "IsLoop", model->GetIsLoopPtr(), true);
}

void PropertyBinder::DrawAnimationModel(const std::string& groupName, const std::string& customLabel)
{
#ifdef IS_DEVELOPMENT
    std::string prefix = groupName + "_";
    auto* gv = GlobalVariables::GetInstance();
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    ImGui::PushID(groupName.c_str());

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.4f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.5f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.6f, 0.4f, 1.0f));

    bool isOpened = ImGui::CollapsingHeader(displayLabel.c_str(), ImGuiTreeNodeFlags_None);
    ImGui::PopStyleColor(3);

    if (isOpened)
    {
        ImGui::Indent(20.0f);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);

        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_FramePadding |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (ImGui::TreeNodeEx("Animation", nodeFlags, "アニメーション設定"))
        {
            ImGui::Spacing();
            Draw(prefix + "SpeedScale", "再生速度");
            Draw(prefix + "IsLoop", "ループ再生");
            ImGui::TreePop();
        }
        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Transform", nodeFlags, "トランスフォーム"))
        {
            ImGui::Spacing();
            Draw(prefix + "Trans", "位置");
            Draw(prefix + "Rot", "回転");
            Draw(prefix + "Scale", "スケール");

            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("UVTransform", nodeFlags, "UV トランスフォーム"))
        {
            ImGui::Spacing();
            Draw(prefix + "UVTrans", "UV 位置");
            Draw(prefix + "UVRot", "UV 回転");
            Draw(prefix + "UVScale", "UV スケール");
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("BasicSettings", nodeFlags, "基本マテリアル設定"))
        {
            ImGui::Spacing();
            ImGui::TextDisabled("パラメーター");
            Draw(prefix + "Color", "カラー");
            Draw(prefix + "Lighting", "ライティング有効");
            Draw(prefix + "LightMode", "照明モード");
            Draw(prefix + "EnvMapInt", "環境マップ強度");
            Draw(prefix + "Emissive", "自己発光強度");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextDisabled("テクスチャ");

            Draw(prefix + "AlbedoMap", "メインテクスチャ");
            Draw(prefix + "EnvMapTex", "環境マップ");
            Draw(prefix + "ToonRampTex", "トゥーンランプ");

            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Surface", nodeFlags, "質感"))
        {
            ImGui::Spacing();

            int currentMode = gv->GetIntValue(groupPath_, prefix + "LightMode");

            bool isPBR = (currentMode == 3);

            if (isPBR)
            {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[ PBR設定 ]");

                Draw(prefix + "Roughness", "粗さ (Roughness)");
                Draw(prefix + "Metalness", "金属度 (Metalness)");
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "[ スタンダード設定 ]");

                Draw(prefix + "Shininess", "光沢度 (Shininess)");
                Draw(prefix + "SpecColor", "スペキュラ色");
                Draw(prefix + "DiffuseRef", "拡散反射率");
            }

            ImGui::Spacing();
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Shadow", nodeFlags, "影設定"))
        {
            ImGui::Spacing();
            Draw(prefix + "AddShadow", "影を受ける (有効化)");

            if (gv->GetIntValue(groupPath_, prefix + "AddShadow") > 0)
            {
                ImGui::Indent(10.0f);

                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "詳細設定:");
                Draw(prefix + "ShadowDens", "影の濃さ");
                Draw(prefix + "ShadowBias", "バイアス調整");
                Draw(prefix + "ShadowSoft", "エッジの柔らかさ");

                ImGui::Unindent(10.0f);
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("RimLight", nodeFlags, "リムライト"))
        {
            ImGui::Spacing();
            Draw(prefix + "RimEnable", "リムライト有効");

            if (gv->GetIntValue(groupPath_, prefix + "RimEnable") > 0)
            {
                ImGui::Indent(10.0f);

                Draw(prefix + "RimColor", "発光色");
                Draw(prefix + "RimInten", "発光強度");
                Draw(prefix + "RimPower", "リムの鋭さ");
                Draw(prefix + "RimUseDir", "ライト方向依存");

                ImGui::Unindent(10.0f);
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("NormalMap", nodeFlags, "法線マップ"))
        {
            ImGui::Spacing();
            Draw(prefix + "NormEnable", "法線マップ有効");

            if (gv->GetIntValue(groupPath_, prefix + "NormEnable") > 0)
            {
                ImGui::Indent(10.0f);

                Draw(prefix + "NormalMapTex", "法線テクスチャ");
                Draw(prefix + "NormInten", "凹凸の強さ");
                Draw(prefix + "NormTile", "タイリング回数");

                ImGui::Unindent(10.0f);
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Dissolve", nodeFlags, "ディゾルブ"))
        {
            ImGui::Spacing();
            Draw(prefix + "DisEnable", "ディゾルブ有効");

            if (gv->GetIntValue(groupPath_, prefix + "DisEnable") > 0)
            {
                ImGui::Indent(10.0f);

                Draw(prefix + "DissolveTex", "ノイズマップ");
                Draw(prefix + "DisThres", "進行度");

                ImGui::Separator();
                ImGui::TextDisabled("Edge Settings");

                Draw(prefix + "EdgeWidth", "エッジ幅");
                Draw(prefix + "EdgeInten", "エッジ発光強度");
                Draw(prefix + "EdgeColor", "エッジ色");

                ImGui::Unindent(10.0f);
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Outline", nodeFlags, "アウトライン"))
        {
            ImGui::Spacing();
            Draw(prefix + "OutlineEnable", "アウトライン有効");

            if (gv->GetBoolValue(groupPath_, prefix + "OutlineEnable"))
            {
                ImGui::Indent(10.0f);

                Draw(prefix + "OutlineWidth", "線の太さ");
                Draw(prefix + "OutlineColor", "線の色");

                ImGui::Unindent(10.0f);
            }
            ImGui::Spacing();
            ImGui::TreePop();
        }

        ImGui::PopItemWidth();

        ImGui::Unindent(20.0f);
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
    BindTexture(prefix + "Tex", sprite->GetTextureHandlePtr());

    BindBool(prefix + "Visible", sprite->GetIsVisiblePtr(), true);
    Bind(prefix + "Layer", sprite->GetLayerOrderPtr(), 0, 1.0f);

    auto onUVChange = [sprite]()
        {
            sprite->UpdateUV();
        };
    Bind(prefix + "UVTrans", &uvTransform.translation_, { 0.0f, 0.0f, 0.0f }, 0.01f, onUVChange);
    BindRotation(prefix + "UVRot", &uvTransform.rotation_, &uvTransform.rotationQuaternion_, 0.01f);
    Bind(prefix + "UVScale", &uvTransform.scale_, { 1.0f, 1.0f, 1.0f }, 0.01f);

    BindBool(prefix + "DisEnable", &mat->enableDissolve, false);
    BindTexture(prefix + "DisTex", sprite->GetDissolveTextureHandlePtr());
    Bind(prefix + "DisThres", &mat->dissolveThreshold, 0.5f, 0.01f, 0.0f, 1.0f);
    Bind(prefix + "EdgeWidth", &mat->edgeWidth, 0.05f, 0.001f, 0.0f, 0.5f);
    Bind(prefix + "EdgeInten", &mat->edgeIntensity, 2.0f, 0.1f, 0.0f, 10.0f);
    BindColor(prefix + "EdgeColor", &mat->edgeColor, { 1.0f, 0.5f, 0.0f });
}

void PropertyBinder::DrawSprite(const std::string& groupName, const std::string& customLabel)
{
#ifdef IS_DEVELOPMENT
    std::string prefix = groupName + "_";
    auto* gv = GlobalVariables::GetInstance();
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    ImGui::PushID(groupName.c_str());

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.4f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.5f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.6f, 0.4f, 1.0f));

    bool isOpened = ImGui::CollapsingHeader(displayLabel.c_str(), ImGuiTreeNodeFlags_None);

    ImGui::PopStyleColor(3);

    if (isOpened)
    {
        ImGui::Indent(20.0f);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);

        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_FramePadding |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        ImGui::Spacing();
        if (ImGui::TreeNodeEx("Settings", nodeFlags, "基本設定"))
        {
            ImGui::Spacing();
            Draw(prefix + "Visible", "表示");

            Draw(prefix + "Layer", "描画順");

            ImGui::TreePop();
        }
        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Transform", nodeFlags, "トランスフォーム"))
        {
            ImGui::Spacing();
            Draw(prefix + "Pos", "位置");
            Draw(prefix + "Size", "スケール");
            Draw(prefix + "Rot", "回転");

            ImGui::Separator();

            Draw(prefix + "Anchor", "アンカーポイント");
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Appearance", nodeFlags, "見た目"))
        {
            ImGui::Spacing();
            Draw(prefix + "Color", "カラー");

            ImGui::Spacing();
            ImGui::TextDisabled("テクスチャ");
            Draw(prefix + "Tex", "メインテクスチャ");

            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("UVTransform", nodeFlags, "UV トランスフォーム"))
        {
            ImGui::Spacing();
            Draw(prefix + "UVTrans", "UV 位置");
            Draw(prefix + "UVRot", "UV 回転");
            Draw(prefix + "UVScale", "UV スケール");
            ImGui::TreePop();
        }

        ImGui::Spacing();

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
        ImGui::Unindent(20.0f);
    }

    ImGui::PopID();
#endif
}

void PropertyBinder::BindTexture(const std::string& key, uint32_t* ptr)
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    // 保存データの整合性を保つ
    gv->AddItem(groupPath_, key, 0);

    // 保存データを取得
    int savedId = gv->GetIntValue(groupPath_, key);

    // 同期処理
    *ptr = TextureHandle::Get(static_cast<TextureID>(savedId));

    // items_ に登録するラムダ式
    items_[key] = [this, ptr, key](const std::string& label)
        {
            GlobalVariables* gv = GlobalVariables::GetInstance();
            int savedId = gv->GetIntValue(groupPath_, key);

            // 現在のID取得
            TextureID currentId = static_cast<TextureID>(savedId);

            // IDが範囲外なら補正
            if ((int)currentId < 0 || (int)currentId >= TEXTURES_COUNT) {
                currentId = white1x1;
            }

            std::string labelName = label.empty() ? key : label;
            auto* srvManager = this->engine_->srvManager_.get();

            // 現在のファイル名を取得
            std::string currentFileName = TextureHandle::GetFileName(currentId);
            if (currentFileName.empty()) currentFileName = "Null / Unknown";

            // ラベル表示
            ImGui::Text("%s", labelName.c_str());

            // 現在のテクスチャハンドルを取得
            uint32_t currentGpuIndex = TextureHandle::Get(currentId);
            auto currentGpuHandle = srvManager->GetSRVHandleGPU(currentGpuIndex);

            std::string popupId = "Popup_" + key;
            bool openPopup = false;

            // ボタン背景色
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

            // プレビューボタン描画
            if (currentGpuHandle.ptr == 0) {
                if (ImGui::Button("Null", ImVec2(32, 32))) { openPopup = true; }
            }
            else {
                if (ImGui::ImageButton(key.c_str(), (ImTextureID)currentGpuHandle.ptr, ImVec2(32, 32),
                    ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
                {
                    openPopup = true;
                }
            }
            ImGui::PopStyleColor();

            // メインボタンのツールチップ
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("[%d] %s", (int)currentId, currentFileName.c_str());
                if (currentGpuHandle.ptr != 0) {
                    ImGui::Image((ImTextureID)currentGpuHandle.ptr, ImVec2(128, 128));
                }
                ImGui::EndTooltip();
            }

            // ボタンの横にファイル名を表示
            ImGui::SameLine(); // 横に並べる
            ImGui::AlignTextToFramePadding(); // テキストの上下位置をボタンに合わせる
            ImGui::TextDisabled("%s", currentFileName.c_str()); // 少しグレーにして表示

            if (openPopup) {
                ImGui::OpenPopup(popupId.c_str());
            }

            // ポップアップ
            ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(FLT_MAX, FLT_MAX));

            if (ImGui::BeginPopup(popupId.c_str()))
            {
                float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                ImGuiStyle& style = ImGui::GetStyle();
                ImVec2 buttonSize(48.0f, 48.0f);

                for (int i = 0; i < TEXTURES_COUNT; i++)
                {
                    TextureID id = static_cast<TextureID>(i);
                    uint32_t hIdx = TextureHandle::Get(id);
                    auto hGPU = srvManager->GetSRVHandleGPU(hIdx);

                    ImGui::PushID(i);

                    bool isSelected = (currentId == id);
                    int pushedColors = 0;

                    if (isSelected) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                        pushedColors++;
                    }
                    else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                        pushedColors++;
                    }

                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.4f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                    pushedColors += 2;

                    if (ImGui::ImageButton("Tex", (ImTextureID)hGPU.ptr, buttonSize,
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
                    {
                        *ptr = static_cast<uint32_t>(id);
                        gv->SetValue(groupPath_, key, static_cast<int>(id));
                        *ptr = TextureHandle::Get(id);
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::PopStyleColor(pushedColors);

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("[%d] %s", i, TextureHandle::GetFileName(id).c_str());
                        if (hGPU.ptr != 0) {
                            ImGui::Image((ImTextureID)hGPU.ptr, ImVec2(128, 128));
                        }
                        ImGui::EndTooltip();
                    }

                    ImGui::PopID();

                    float lastButtonX2 = ImGui::GetItemRectMax().x;
                    float nextButtonX2 = lastButtonX2 + style.ItemSpacing.x + buttonSize.x + (style.FramePadding.x * 2);

                    if (nextButtonX2 < windowVisibleX2 && i < TEXTURES_COUNT - 1)
                    {
                        ImGui::SameLine();
                    }
                }
                ImGui::EndPopup();
            }
        };
}

void PropertyBinder::BindBool(const std::string& key, int32_t* ptr, bool defaultValue)
{
    // boolの初期値をintに変換
    int32_t intDefault = defaultValue ? 1 : 0;

    // 初期値として登録
    RegisterItem(key, intDefault, ptr);

    // セーブデータから値を読み込む
    *ptr = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);

#ifdef IS_DEVELOPMENT
    items_[key] = [=](const std::string& nameOverride)
        {
            std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

            // 変数の値をboolに変換して判定
            bool isChecked = (*ptr != 0);

            if (ImGui::Checkbox(label.c_str(), &isChecked))
            {
                // チェック結果を0 or 1に戻して保存
                *ptr = isChecked ? 1 : 0;
                GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
            }
        };
#endif
}

// uint32_tをチェックボックスとしてバインド
void PropertyBinder::BindBool(const std::string& key, uint32_t* ptr, bool defaultValue)
{
    // boolの初期値をintに変換
    int32_t intDefault = defaultValue ? 1 : 0;

    // int型としてキャストして登録
    RegisterItem(key, intDefault, reinterpret_cast<int32_t*>(ptr));

    // ロード
    int32_t loadedVal = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);
    *ptr = static_cast<uint32_t>(loadedVal);

#ifdef IS_DEVELOPMENT
    items_[key] = [=](const std::string& nameOverride)
        {
            std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

            bool isChecked = (*ptr != 0);

            if (ImGui::Checkbox(label.c_str(), &isChecked))
            {
                *ptr = isChecked ? 1 : 0;
                GlobalVariables::GetInstance()->SetValue(groupPath_, key, static_cast<int32_t>(*ptr));
            }
        };
#endif
}

void PropertyBinder::BindRotation(const std::string& key, Vector3* eulerPtr, Quaternion* quatPtr, float speed, std::function<void()> onChange)
{
    // BindVector3Internalを呼び出す
    BindVector3Internal(key, eulerPtr, { 0.0f, 0.0f, 0.0f }, speed, 0.0f, 360.0f,
        [eulerPtr, quatPtr, onChange]()
        {
            //オイラー角からクォータニオンへ変換
            *quatPtr = Quaternion::QuaternionFromEuler(*eulerPtr);

            // 追加の処理
            if (onChange)
            {
                onChange();
            }
        }
    );
}

void PropertyBinder::BindCombo(const std::string& key, int32_t* ptr, int32_t defaultValue, const char* items)
{
    // データの登録と読み込み
    RegisterItem(key, defaultValue, ptr);
    *ptr = GlobalVariables::GetInstance()->GetIntValue(groupPath_, key);

#ifdef IS_DEVELOPMENT
    // 描画関数の登録
    items_[key] = [=](const std::string& nameOverride)
        {
            // 名前が指定されていればそれを使い、なければキーを使う + ID重複防止
            std::string label = (nameOverride.empty() ? key : nameOverride) + "###" + key;

            // ImGui::Combo を実行
            if (ImGui::Combo(label.c_str(), ptr, items))
            {
                // 変更があったら保存
                GlobalVariables::GetInstance()->SetValue(groupPath_, key, *ptr);
            }
        };
#endif
}