#include "pch.h"
#include "PropertyBinder.h"
#include "Model.h"
#include "AnimationModel.h"
#include "Sprite.h"
#include "Engine.h"
#include "SRVManager.h"

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
                            Draw(matPrefix + "UseTriplanar", "トライプラナー有効 (ワールド座標投影)");

                            if (gv->GetIntValue(groupPath_, matPrefix + "UseTriplanar") > 0)
                            {
                                ImGui::Spacing();
                                ImGui::TextDisabled("トライプラナー設定 (通常のUVは無視)");
                                Draw(matPrefix + "TriScale", "テクスチャスケール");
                                Draw(matPrefix + "TriSharpness", "ブレンドのシャープさ");
                            }
                            else
                            {
                                ImGui::Spacing();
                                ImGui::TextDisabled("標準UV設定");
                                Draw(matPrefix + "UVTrans", "UV 位置");
                                Draw(matPrefix + "UVRot", "UV 回転");
                                Draw(matPrefix + "UVScale", "UV スケール");
                            }

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
                            Draw(matPrefix + "AlphaThres", "透過カット閾値(AlphaTest)");

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

                        if (ImGui::TreeNode("WaterEffects", "水たまり・波紋"))
                        {
                            ImGui::Spacing();
                            Draw(matPrefix + "RippleEnable", "有効化");

                            if (gv->GetIntValue(groupPath_, matPrefix + "RippleEnable") > 0)
                            {
                                Draw(matPrefix + "UsePuddle", "水たまりを形成する");
                                Draw(matPrefix + "Wetness", "濡れ具合 / 水位");
                                Draw(matPrefix + "WetDarkness", "濡れた所の暗さ");

                                ImGui::Separator();
                                ImGui::TextDisabled("波紋設定");
                                Draw(matPrefix + "RippleMap", "波紋法線マップ");
                                Draw(matPrefix + "RippleScale", "雨の密度 (格子数)");
                                Draw(matPrefix + "RippleSize", "一粒の大きさ");      
                                Draw(matPrefix + "RippleFreq", "発生頻度");          
                                Draw(matPrefix + "RippleSpeed", "波の広がる速さ");
                                Draw(matPrefix + "RippleStren", "波紋の高さ(強さ)");
                                Draw(matPrefix + "RippleMix", "レイヤー合成比率");

                                if (gv->GetIntValue(groupPath_, matPrefix + "UsePuddle") > 0)
                                {
                                    ImGui::Separator();
                                    ImGui::TextDisabled("水たまり形状設定");
                                    Draw(matPrefix + "PuddleNoise", "分布ノイズ");
                                    Draw(matPrefix + "PuddleScale", "水たまりスケール");
                                    Draw(matPrefix + "PuddleFalloff", "フチのボケ具合");

                                    ImGui::Spacing();
                                    Draw(matPrefix + "PuddleColor", "水の色と濁り");
                                    Draw(matPrefix + "PuddleEmission", "水の発光強度");
                                }
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

    BindBool(prefix + "Visible", sprite->GetIsVisiblePtr(), true);
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
    TextureType filterType)
{
    // ポインタがnullなら何もしない（安全対策）
    if (!currentNamePtr || !currentHandlePtr) return;

    // 自動的にラムダ式を作成して渡す
    BindTexture(
        key,              // キー
        *currentNamePtr,  // 現在の値

        // 変更があった時の処理を定義
        [currentNamePtr, currentHandlePtr](const std::string& newName)
        {
            // ポインタ先の変数を更新
            *currentNamePtr = newName;

            // ハンドル更新
            *currentHandlePtr = TextureManager::GetInstance().Get(newName);
        },

        defaultName,      // デフォルト値
        filterType        // フィルタ
    );
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