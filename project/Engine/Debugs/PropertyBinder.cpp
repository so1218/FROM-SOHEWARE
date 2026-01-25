#include "PropertyBinder.h"
#include "Model.h"
#include "AnimationModel.h"
#include "Sprite.h"

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
            // 複数あるなら Matを付ける
            matPrefix = prefix + "Mat" + std::to_string(i) + "_";
        }

        // ヘルパー関数を呼び出す
        BindMaterialProperties(matPrefix, model->GetMaterialHandle(i));
    }
}

void PropertyBinder::DrawModel(const std::string& groupName, const std::string& customLabel)
{
#ifdef IS_DEVELOPMENT
    // 基本設定とラベルの準備
    std::string prefix = groupName + "_";
    auto* gv = GlobalVariables::GetInstance();
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    // マップから対象のモデルを取得
    Model* targetModel = nullptr;
    if (modelBindMap_.find(groupName) != modelBindMap_.end())
    {
        targetModel = modelBindMap_[groupName].model;
    }

    // IDの衝突を防ぐためにPushID
    ImGui::PushID(groupName.c_str());

    // ヘッダーの色設定
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.4f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.5f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.6f, 0.4f, 1.0f));

    // メインの折りたたみヘッダー
    bool isOpened = ImGui::CollapsingHeader(displayLabel.c_str(), ImGuiTreeNodeFlags_None);

    ImGui::PopStyleColor(3);

    if (isOpened)
    {
        ImGui::Indent(20.0f);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);

        // ツリーノードの共通フラグ
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

        if (targetModel)
        {
            // 全てのマテリアル設定をまとめる親ノード
            if (ImGui::TreeNodeEx("Materials", nodeFlags, "マテリアル設定"))
            {
                ImGui::Indent(10.0f);

                if (ImGui::TreeNode("BatchSettings", "一括操作 (全適用)"))
                {
                    ImGui::Spacing();


                    static Vector4 batchColor = { 1.0f, 1.0f, 1.0f, 1.0f };
                    ImGui::ColorEdit4("Batch Color", &batchColor.x);

                    if (ImGui::Button("Apply Color to All Materials", ImVec2(-1, 0)))
                    {
                        targetModel->SetColor(batchColor);
                    }

                    ImGui::Spacing();

                    static bool batchOutlineEnable = false;
                    if (ImGui::Checkbox("Enable Outline All", &batchOutlineEnable))
                    {
                        targetModel->SetEnableOutline(batchOutlineEnable);
                    }

                    static float batchOutlineWidth = 1.0f;
                    if (ImGui::DragFloat("Outline Width All", &batchOutlineWidth, 0.1f, 0.0f, 10.0f))
                    {
                        targetModel->SetOutlineWidth(batchOutlineWidth);
                    }

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                size_t matCount = targetModel->GetMaterialCount();
                for (size_t i = 0; i < matCount; ++i)
                {
                    std::string matPrefix;
                    std::string matNodeName;

                    // マテリアル数に応じて名前を変える
                    if (matCount == 1)
                    {
                        matPrefix = prefix;
                        matNodeName = "Material Property";
                    }
                    else
                    {
                        matPrefix = prefix + "Mat" + std::to_string(i) + "_";
                        matNodeName = "Material " + std::to_string(i);
                    }

                    // マテリアルごとのツリー
                    if (ImGui::TreeNodeEx(matNodeName.c_str(), ImGuiTreeNodeFlags_None))
                    {
                        ImGui::Indent(10.0f);

                        if (ImGui::TreeNode("UV Settings", "UV トランスフォーム"))
                        {
                            Draw(matPrefix + "UVTrans", "UV 位置");
                            Draw(matPrefix + "UVRot", "UV 回転");
                            Draw(matPrefix + "UVScale", "UV スケール");
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("BasicSettings", "基本設定"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "Color", "カラー");
                            Draw(matPrefix + "Lighting", "ライティング有効");
                            Draw(matPrefix + "LightMode", "照明モード");
                            Draw(matPrefix + "EnvMapInt", "環境マップ強度");
                            Draw(matPrefix + "Emissive", "自己発光強度");

                            ImGui::Separator();
                            ImGui::TextDisabled("テクスチャ");
                            Draw(matPrefix + "AlbedoMap", "メインテクスチャ");
                            Draw(matPrefix + "EnvMapTex", "環境マップ");
                            Draw(matPrefix + "ToonRampTex", "トゥーンランプ");

                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Surface", "質感"))
                        {
                            ImGui::Spacing();
                            int currentMode = gv->GetIntValue(groupPath_, matPrefix + "LightMode");
                            bool isPBR = (currentMode == 3);

                            if (isPBR)
                            {
                                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[ PBR設定 ]");
                                Draw(matPrefix + "Roughness", "粗さ");
                                Draw(matPrefix + "Metalness", "金属度");
                            }
                            else
                            {
                                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "[ スタンダード設定 ]");
                                Draw(matPrefix + "Shininess", "光沢度");
                                Draw(matPrefix + "SpecColor", "スペキュラ色");
                                Draw(matPrefix + "DiffuseRef", "拡散反射率");
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Shadow", "影設定"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "AddShadow", "影を受ける");
                            if (gv->GetIntValue(groupPath_, matPrefix + "AddShadow") > 0)
                            {
                                Draw(matPrefix + "ShadowDens", "影のキレ(閾値)");
                                Draw(matPrefix + "ShadowEnv", "影の明るさ");
                                Draw(matPrefix + "ShadowBias", "バイアス");
                                Draw(matPrefix + "ShadowSoft", "柔らかさ");
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("RimLight", "リムライト"))
                        {
                            ImGui::Spacing();
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

                        if (ImGui::TreeNode("NormalMap", "法線マップ"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "NormEnable", "有効化");
                            if (gv->GetIntValue(groupPath_, matPrefix + "NormEnable") > 0)
                            {
                                Draw(matPrefix + "NormalMapTex", "テクスチャ");
                                Draw(matPrefix + "NormInten", "凹凸の強さ");
                                Draw(matPrefix + "NormTile", "タイリング");
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Dissolve", "ディゾルブ"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "DisEnable", "有効化");
                            if (gv->GetIntValue(groupPath_, matPrefix + "DisEnable") > 0)
                            {
                                Draw(matPrefix + "DissolveTex", "ノイズマップ");
                                Draw(matPrefix + "DisThres", "進行度");
                                ImGui::Separator();
                                Draw(matPrefix + "EdgeWidth", "エッジ幅");
                                Draw(matPrefix + "EdgeInten", "エッジ強度");
                                Draw(matPrefix + "EdgeColor", "エッジ色");
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Outline", "アウトライン"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "OutlineEnable", "有効化");
                            if (gv->GetIntValue(groupPath_, matPrefix + "OutlineEnable") > 0)
                            {
                                Draw(matPrefix + "OutlineWidth", "線の太さ");
                                Draw(matPrefix + "OutlineColor", "線の色");
                            }
                            ImGui::TreePop();
                        }

                        ImGui::Unindent(10.0f);
                        ImGui::TreePop();
                    }
                }

                ImGui::Unindent(10.0f);
                ImGui::TreePop();
            }
        }

        ImGui::Spacing();
        ImGui::PopItemWidth();
        ImGui::Unindent(20.0f);
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
    BindBool(prefix + "IsLoop", model->GetIsLoopPtr(), true);
}

void PropertyBinder::DrawAnimationModel(const std::string& groupName, const std::string& customLabel)
{
#ifdef IS_DEVELOPMENT
    // 基本設定とラベルの準備
    std::string prefix = groupName + "_";
    auto* gv = GlobalVariables::GetInstance();
    std::string displayLabel = customLabel.empty() ? groupName : customLabel;

    // マップから対象のモデルを取得
    AnimationModel* targetModel = nullptr;
    if (animationBindMap_.find(groupName) != animationBindMap_.end())
    {
        targetModel = animationBindMap_[groupName].model;
    }

    // IDの衝突を防ぐためにPushID
    ImGui::PushID(groupName.c_str());

    // ヘッダーの色設定
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.4f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.5f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.6f, 0.4f, 1.0f));

    // メインの折りたたみヘッダー
    bool isOpened = ImGui::CollapsingHeader(displayLabel.c_str(), ImGuiTreeNodeFlags_None);

    ImGui::PopStyleColor(3);

    if (isOpened)
    {
        ImGui::Indent(20.0f);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);

        // ツリーノードの共通フラグ
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

        if (targetModel)
        {
            // 親ノードで括る
            if (ImGui::TreeNodeEx("Materials", nodeFlags, "マテリアル設定"))
            {
                ImGui::Indent(10.0f);

                if (ImGui::TreeNode("BatchSettings", "一括操作 (全適用)"))
                {
                    ImGui::Spacing();

                    static Vector4 batchColor = { 1.0f, 1.0f, 1.0f, 1.0f };
                    ImGui::ColorEdit4("Batch Color", &batchColor.x);

                    if (ImGui::Button("Apply Color to All Materials", ImVec2(-1, 0)))
                    {
                        targetModel->SetColor(batchColor);
                    }

                    ImGui::Spacing();

                    static bool batchOutlineEnable = false;
                    if (ImGui::Checkbox("Enable Outline All", &batchOutlineEnable))
                    {
                        targetModel->SetEnableOutline(batchOutlineEnable);
                    }

                    static float batchOutlineWidth = 1.0f;
                    if (ImGui::DragFloat("Outline Width All", &batchOutlineWidth, 0.1f, 0.0f, 10.0f))
                    {
                        targetModel->SetOutlineWidth(batchOutlineWidth);
                    }

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                size_t matCount = targetModel->GetMaterialCount();
                for (size_t i = 0; i < matCount; ++i)
                {
                    std::string matPrefix;
                    std::string matNodeName;

                    if (matCount == 1)
                    {
                        matPrefix = prefix;
                        matNodeName = "Material Property";
                    }
                    else
                    {
                        matPrefix = prefix + "Mat" + std::to_string(i) + "_";
                        matNodeName = "Material " + std::to_string(i);
                    }

                    // 個別マテリアルのツリー
                    if (ImGui::TreeNodeEx(matNodeName.c_str(), ImGuiTreeNodeFlags_None))
                    {
                        ImGui::Indent(10.0f);

                        if (ImGui::TreeNode("UV Settings", "UV トランスフォーム"))
                        {
                            Draw(matPrefix + "UVTrans", "UV 位置");
                            Draw(matPrefix + "UVRot", "UV 回転");
                            Draw(matPrefix + "UVScale", "UV スケール");
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("BasicSettings", "基本設定"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "Color", "カラー");
                            Draw(matPrefix + "Lighting", "ライティング有効");
                            Draw(matPrefix + "LightMode", "照明モード");
                            Draw(matPrefix + "EnvMapInt", "環境マップ強度");
                            Draw(matPrefix + "Emissive", "自己発光強度");

                            ImGui::Separator();
                            ImGui::TextDisabled("テクスチャ");
                            Draw(matPrefix + "AlbedoMap", "メインテクスチャ");
                            Draw(matPrefix + "EnvMapTex", "環境マップ");
                            Draw(matPrefix + "ToonRampTex", "トゥーンランプ");

                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Surface", "質感"))
                        {
                            ImGui::Spacing();
                            int currentMode = gv->GetIntValue(groupPath_, matPrefix + "LightMode");
                            if (currentMode == 3)
                            {
                                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[ PBR ]");
                                Draw(matPrefix + "Roughness", "粗さ");
                                Draw(matPrefix + "Metalness", "金属度");
                            }
                            else
                            {
                                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "[ Standard ]");
                                Draw(matPrefix + "Shininess", "光沢度");
                                Draw(matPrefix + "SpecColor", "スペキュラ色");
                                Draw(matPrefix + "DiffuseRef", "拡散反射率");
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Shadow", "影設定"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "AddShadow", "影を受ける");
                            if (gv->GetIntValue(groupPath_, matPrefix + "AddShadow") > 0)
                            {
                                Draw(matPrefix + "ShadowDens", "影の濃さ");
                                Draw(matPrefix + "ShadowBias", "バイアス");
                                Draw(matPrefix + "ShadowSoft", "柔らかさ");
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("RimLight", "リムライト"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "RimEnable", "有効化");
                            if (gv->GetIntValue(groupPath_, matPrefix + "RimEnable") > 0)
                            {
                                Draw(matPrefix + "RimColor", "発光色");
                                Draw(matPrefix + "RimInten", "強度");
                                Draw(matPrefix + "RimPower", "鋭さ");
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("NormalMap", "法線マップ"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "NormEnable", "有効化");
                            if (gv->GetIntValue(groupPath_, matPrefix + "NormEnable") > 0)
                            {
                                Draw(matPrefix + "NormalMapTex", "テクスチャ");
                                Draw(matPrefix + "NormInten", "強度");
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Dissolve", "ディゾルブ"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "DisEnable", "有効化");
                            if (gv->GetIntValue(groupPath_, matPrefix + "DisEnable") > 0)
                            {
                                Draw(matPrefix + "DissolveTex", "ノイズマップ");
                                Draw(matPrefix + "DisThres", "進行度");
                                Draw(matPrefix + "EdgeWidth", "エッジ幅");
                                Draw(matPrefix + "EdgeColor", "エッジ色");
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Outline", "アウトライン"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "OutlineEnable", "有効化");
                            if (gv->GetIntValue(groupPath_, matPrefix + "OutlineEnable") > 0)
                            {
                                Draw(matPrefix + "OutlineWidth", "線の太さ");
                                Draw(matPrefix + "OutlineColor", "線の色");
                            }
                            ImGui::TreePop();
                        }

                        ImGui::Unindent(10.0f);
                        ImGui::TreePop();
                    }
                }

                ImGui::Unindent(10.0f);
                ImGui::TreePop();
            }
        }

        ImGui::Spacing();
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
    BindTexture(prefix + "Tex", sprite->GetTextureHandlePtr(), TextureID::white1x1);

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
    BindTexture(prefix + "DisTex", sprite->GetDissolveTextureHandlePtr(), TextureID::white1x1, TextureType::Noise);
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

void PropertyBinder::BindTexture(const std::string& key, uint32_t* ptr, TextureID defaultId, TextureType filterType)
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    // 保存データの整合性を保つ
    gv->AddItem(groupPath_, key, static_cast<int>(defaultId));

    // 保存データを取得
    int savedId = gv->GetIntValue(groupPath_, key);

    // 同期処理
    *ptr = TextureHandle::Get(static_cast<TextureID>(savedId));

    items_[key] = [this, ptr, key, filterType](const std::string& label)
        {
            GlobalVariables* gv = GlobalVariables::GetInstance();
            int savedId = gv->GetIntValue(groupPath_, key);

            TextureID currentId = static_cast<TextureID>(savedId);

            if ((int)currentId < 0 || (int)currentId >= TEXTURES_COUNT) {
                currentId = white1x1;
            }

            std::string labelName = label.empty() ? key : label;
            auto* srvManager = this->engine_->srvManager_.get();

            std::string currentFileName = TextureHandle::GetFileName(currentId);
            if (currentFileName.empty()) currentFileName = "Null / Unknown";

            // 現在のタイプがキューブマップかどうか判定
            bool isCurrentCubeMap = (TextureHandle::GetType(currentId) == TextureType::CubeMap);

#ifdef IS_DEVELOPMENT
            // ラベル表示
            ImGui::Text("%s", labelName.c_str());

            uint32_t currentGpuIndex = TextureHandle::Get(currentId);
            auto currentGpuHandle = srvManager->GetSRVHandleGPU(currentGpuIndex);

            std::string popupId = "Popup_" + key;
            bool openPopup = false;

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

            // メインボタンの分岐処理
            if (currentGpuHandle.ptr == 0)
            {
                if (ImGui::Button("Null", ImVec2(32, 32))) { openPopup = true; }
            }
            else if (isCurrentCubeMap)
            {
                // キューブマップの場合は画像を使わず、テキストボタンで代用
                if (ImGui::Button("CUBE", ImVec2(32, 32)))
                {
                    openPopup = true;
                }
            }
            else
            {
                // 通常テクスチャなら画像を表示
                if (ImGui::ImageButton(key.c_str(), (ImTextureID)currentGpuHandle.ptr, ImVec2(32, 32),
                    ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
                {
                    openPopup = true;
                }
            }

            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("[%d] %s", (int)currentId, currentFileName.c_str());

                // ツールチップでもキューブマップなら画像を出さない
                if (currentGpuHandle.ptr != 0 && !isCurrentCubeMap) {
                    ImGui::Image((ImTextureID)currentGpuHandle.ptr, ImVec2(128, 128));
                }
                else if (isCurrentCubeMap) {
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "< Cubemap Texture >");
                }
                ImGui::EndTooltip();
            }

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%s", currentFileName.c_str());

            if (openPopup) {
                ImGui::OpenPopup(popupId.c_str());
            }

            ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(FLT_MAX, FLT_MAX));

            if (ImGui::BeginPopup(popupId.c_str()))
            {
                float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                ImGuiStyle& style = ImGui::GetStyle();
                ImVec2 buttonSize(48.0f, 48.0f);
                int displayedCount = 0;

                for (int i = 0; i < TEXTURES_COUNT; i++)
                {
                    TextureID id = static_cast<TextureID>(i);

                    // フィルタリング
                    if (TextureHandle::GetType(id) != filterType && id != currentId) {
                        continue;
                    }

                    // このアイテムがキューブマップか判定
                    bool isItemCubeMap = (TextureHandle::GetType(id) == TextureType::CubeMap);

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

                    bool clicked = false;

                    // ポップアップ内の分岐処理
                    if (isItemCubeMap) {
                        // キューブマップならテキストボタン
                        if (ImGui::Button("DDS\nCUBE", buttonSize))
                        {
                            clicked = true;
                        }
                    }
                    else {
                        // 通常テクスチャなら画像ボタン
                        if (ImGui::ImageButton("Tex", (ImTextureID)hGPU.ptr, buttonSize,
                            ImVec2(0, 0), ImVec2(1, 1),
                            ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
                        {
                            clicked = true;
                        }
                    }

                    if (clicked)
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

                        // ツールチップでも分岐
                        if (hGPU.ptr != 0 && !isItemCubeMap) {
                            ImGui::Image((ImTextureID)hGPU.ptr, ImVec2(128, 128));
                        }
                        else if (isItemCubeMap) {
                            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Skybox / Cubemap");
                        }
                        ImGui::EndTooltip();
                    }

                    ImGui::PopID();

                    float lastButtonX2 = ImGui::GetItemRectMax().x;
                    float nextButtonX2 = lastButtonX2 + style.ItemSpacing.x + buttonSize.x + (style.FramePadding.x * 2);

                    if (nextButtonX2 < windowVisibleX2)
                    {
                        ImGui::SameLine();
                    }
                    displayedCount++;
                }

                if (displayedCount == 0)
                {
                    ImGui::TextDisabled("No textures found.");
                }

                ImGui::EndPopup();
            }
#endif
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

// マテリアルのプロパティを登録する関数
void PropertyBinder::BindMaterialProperties(const std::string& prefix, MaterialHandle* handle)
{
    if (!handle || !handle->materialData) return;

    MaterialData* matData = handle->materialData;

    BindTexture(prefix + "AlbedoMap", &handle->textureHandle, TextureID::white1x1, TextureType::Albedo);
    BindTexture(prefix + "EnvMapTex", &handle->envMapHandle, TextureID::skyboxCubemap, TextureType::CubeMap);
    BindTexture(prefix + "NormalMapTex", &handle->normalMapHandle, TextureID::normal_01, TextureType::Normal);
    BindTexture(prefix + "DissolveTex", &handle->dissolveMapHandle, TextureID::white1x1, TextureType::Noise);
    BindTexture(prefix + "ToonRampTex", &handle->toonRampHandle, TextureID::toonRamp, TextureType::Toon);

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
}