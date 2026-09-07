#include "pch.h"
#include "WaterObject.h"

using namespace FE;

WaterObject::WaterObject(Engine* engine, int id, const std::string& parentGroupName)
    : engine_(engine), id_(id), parentGroupName_(parentGroupName)
{
    // 水面用のメッシュモデルを生成
    model_ = std::make_unique<Model>(engine_, "water");
}

void WaterObject::Initialize()
{
    // 個別の定数バッファを作成
    materialCBResource_ = BufferManager::CreateMappedConstantBuffer<WaterMaterialData>(
        engine_->GetGraphicsDevice()->GetDevice(),
        &mappedMaterialData_
    );

    // プロパティのバインドを実行 
    BindProperties();
}

void WaterObject::BindProperties()
{
    std::string childGroupName = "Water_" + std::to_string(id_);
    binder_ = std::make_unique<PropertyBinder>(engine_, parentGroupName_, childGroupName);

    binder_->Bind("Position", &model_->GetTransform().translation_, Vector3(0.0f, 0.0f, 0.0f));
    binder_->BindRotation("Rotation", &model_->GetTransform().rotation_, &model_->GetTransform().rotationQuaternion_, 0.01f);
    binder_->Bind("Scale", &model_->GetTransform().scale_, Vector3(10.0f, 1.0f, 10.0f));

    binder_->BindTexture("NormalMap", &normalMapName_, &normalMapHandle_, "white1x1", TextureType::Normal);
    binder_->BindTexture("RippleTexture", &rippleTextureName_, &rippleTextureHandle_, "white1x1", TextureType::Normal);
    binder_->BindTexture("EnvMap", &envMapName_, &envMapSrvHandle_, "pureSky", TextureType::CubeMap);

    binder_->Bind("GlobalWindDirection", &materialData_.globalWindDirection, Vector2(1.0f, 0.5f));
    binder_->Bind("WaveLength", &materialData_.waveLength, 15.0f, 0.1f, 0.5f, 100.0f);
    binder_->Bind("WaveAmplitude", &materialData_.waveAmplitude, 0.5f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("WaveSteepness", &materialData_.waveSteepness, 0.4f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("WaveSpeed", &materialData_.waveSpeed, 1.0f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("WaveAmplitudeFalloff", &materialData_.waveAmplitudeFalloff, 0.5f, 0.01f, 0.1f, 1.0f);
    binder_->Bind("WaveLengthFalloff", &materialData_.waveLengthFalloff, 0.5f, 0.05f, 0.1f, 1.0f);

    binder_->Bind("WaveDirectionSpread", &materialData_.waveDirectionSpread, 0.3f, 0.01f, 0.0f, 1.57f);
    binder_->Bind("InteractionHeightScale", &materialData_.interactionHeightScale, 1.0f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("InteractionSinkForce", &materialData_.interactionSinkForce, 0.4f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("InteractionBulgeForce", &materialData_.interactionBulgeForce, 0.15f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("InteractionNormalScale", &materialData_.interactionNormalScale, 1.5f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("InteractionFoamIntensity", &materialData_.interactionFoamIntensity, 1.2f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("NormalTiling", &materialData_.normalTiling, Vector2(0.1f, 0.1f));

    binder_->BindColor("ShallowColor", &materialData_.shallowColor, Vector4(0.1f, 0.6f, 0.8f, 0.8f));
    binder_->BindColor("DeepColor", &materialData_.deepColor, Vector4(0.0f, 0.1f, 0.3f, 1.0f));
    binder_->BindColor("ScatterColor", &materialData_.scatterColor, Vector4(0.0f, 0.4f, 0.3f, 1.0f));
    binder_->BindColor("FoamColor", &materialData_.foamColor, Vector4(0.95f, 0.98f, 1.0f, 0.9f));

    binder_->Bind("Absorption", &materialData_.absorption, 0.5f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("RefractionAmount", &materialData_.refractionAmount, 0.05f, 0.001f, 0.0f, 0.5f);
    binder_->Bind("ChromaticAberration", &materialData_.chromaticAberration, 0.5f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("NormalIntensity", &materialData_.normalIntensity, 0.7f, 0.05f, 0.0f, 1.0f);
    binder_->Bind("Roughness", &materialData_.roughness, 0.1f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("SpecularIntensity", &materialData_.specularIntensity, 1.0f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("EnvReflectionIntensity", &materialData_.envReflectionIntensity, 1.0f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("SSRIntensity", &materialData_.ssrIntensity, 1.0f, 0.05f, 0.0f, 2.0f);
    binder_->Bind("SSRDistortion", &materialData_.ssrDistortion, 0.04f, 0.005f, 0.0f, 0.2f);           
    binder_->Bind("SSRStepSize", &materialData_.ssrStepSize, 0.15f, 0.01f, 0.01f, 1.0f);
    binder_->Bind("SSRMaxDistance", &materialData_.ssrMaxDistance, 50.0f, 1.0f, 5.0f, 200.0f);
    binder_->Bind("SSRThickness", &materialData_.ssrThickness, 0.5f, 0.01f, 0.05f, 5.0f);
    binder_->Bind("SSRMaxSteps", &materialData_.ssrMaxSteps, 64.0f, 1.0f, 8.0f, 128.0f);                
    binder_->Bind("SSRBinarySearchSteps", &materialData_.ssrBinarySearchSteps, 8.0f, 1.0f, 0.0f, 16.0f);

    binder_->Bind("WaveFoamThreshold", &materialData_.waveFoamThreshold, 0.25f, 0.01f, 0.01f, 1.0f);
    binder_->Bind("ShoreFoamThreshold", &materialData_.shoreFoamThreshold, 0.3f, 0.01f, 0.01f, 2.0f);
    binder_->Bind("FoamScale", &materialData_.foamScale, 0.5f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("FoamIntensity", &materialData_.foamIntensity, 1.0f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("CausticsIntensity", &materialData_.causticsIntensity, 0.5f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("CausticsScale", &materialData_.causticsScale, 1.0f, 0.05f, 0.1f, 10.0f);
    binder_->Bind("CausticsSpeed", &materialData_.causticsSpeed, 0.05f, 0.01f, 0.0f, 0.5f);
    binder_->Bind("CausticsDistortion", &materialData_.causticsDistortion, 1.0f, 0.05f, 0.0f, 3.0f);    
}

void WaterObject::SetId(int newId)
{
    id_ = newId;
    BindProperties(); // 新しい ID (Water_X) でバインダーを再作成
}

void WaterObject::Update()
{
    model_->GetTransform().UpdateMatrix();

    // UIや自動計算で変更されたマテリアル値をGPU側の定数バッファへ同期
    if (mappedMaterialData_)
    {
        *mappedMaterialData_ = materialData_;
    }
}

void WaterObject::Draw()
{
    if (!model_ || !model_->GetModelData()) return;

    // レンダーマネージャーへ水オブジェクトを描画登録
    engine_->GetRendererManager()->SubmitWater(
        model_->GetTransform(),
        *model_->GetModelData(),
        materialCBResource_->GetGPUVirtualAddress(),
        normalMapHandle_,
        rippleTextureHandle_,
        envMapSrvHandle_
    );
}

void WaterObject::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::PushID(id_);

    std::string headerName = "水面 (Water " + std::to_string(id_) + ")";

    if (ImGui::CollapsingHeader(headerName.c_str()))
    {
        ImGui::Indent();

        if (ImGui::TreeNode("トランスフォーム"))
        {
            binder_->Draw("Position", "座標");
            binder_->Draw("Rotation", "回転");
            binder_->Draw("Scale", "スケール");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("テクスチャ設定"))
        {
            binder_->Draw("NormalMap", "法線マップ");
            binder_->Draw("RippleTexture", "波紋テクスチャ");
            binder_->Draw("EnvMap", "環境マップ");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("基本形状"))
        {
            binder_->Draw("GlobalWindDirection", "風向き");
            binder_->Draw("WaveLength", "基本波長 (m)");
            binder_->Draw("WaveAmplitude", "基本振幅");
            binder_->Draw("WaveSteepness", "波頭の鋭さ");
            binder_->Draw("WaveSpeed", "進行速度");
            binder_->Draw("WaveAmplitudeFalloff", "高次波の振幅減衰率");
            binder_->Draw("WaveLengthFalloff", "高次波の波長縮小率");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("インタラクション"))
        {
            binder_->Draw("WaveDirectionSpread", "方向分散角度 (rad)");
            binder_->Draw("InteractionHeightScale", "高さ変形全体スケール");
            binder_->Draw("InteractionSinkForce", "足元の沈み込み強度");
            binder_->Draw("InteractionBulgeForce", "周囲波の盛り上がり強度");
            binder_->Draw("InteractionNormalScale", "波紋法線歪み強度");
            binder_->Draw("InteractionFoamIntensity", "移動痕跡の泡強度");
            binder_->Draw("NormalTiling", "法線タイリング (xy)");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("カラー設定"))
        {
            binder_->Draw("ShallowColor", "浅瀬の色");
            binder_->Draw("DeepColor", "深い場所の色");
            binder_->Draw("ScatterColor", "水中散乱光の色 (SSS)");
            binder_->Draw("FoamColor", "泡の色・透明度");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("光学・ライティング & SSR"))
        {
            binder_->Draw("Absorption", "吸光度 (Beer-Lambert)");
            binder_->Draw("RefractionAmount", "屈折歪み強度");
            binder_->Draw("ChromaticAberration", "色収差強度");
            binder_->Draw("NormalIntensity", "法線マップ適用強度");
            binder_->Draw("Roughness", "ラフネス");
            binder_->Draw("SpecularIntensity", "ハイライト強度");
            binder_->Draw("EnvReflectionIntensity", "環境反射強度");
            binder_->Draw("SSRIntensity", "SSR反射強度");
            binder_->Draw("SSRDistortion", "SSR法線歪み強度");        
            binder_->Draw("SSRStepSize", "SSR初期ステップ幅");
            binder_->Draw("SSRMaxDistance", "SSR最大描画距離");
            binder_->Draw("SSRThickness", "SSR判定厚み");
            binder_->Draw("SSRMaxSteps", "SSR最大ステップ数");         
            binder_->Draw("SSRBinarySearchSteps", "SSR二分探索ステップ数"); 
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("泡 & コースティクス"))
        {
            binder_->Draw("WaveFoamThreshold", "波頭の泡発生閾値");
            binder_->Draw("ShoreFoamThreshold", "岸辺の泡発生水深");
            binder_->Draw("FoamScale", "泡ノイズ・スケール");
            binder_->Draw("FoamIntensity", "泡の全体濃度");
            binder_->Draw("CausticsIntensity", "コースティクス・強度");
            binder_->Draw("CausticsScale", "コースティクス・スケール");
            binder_->Draw("CausticsSpeed", "コースティクス揺らぎ速度");
            binder_->Draw("CausticsDistortion", "コースティクス波紋歪み強度"); 
            ImGui::TreePop();
        }

        ImGui::Unindent();
        ImGui::Spacing();
    }

    ImGui::PopID();
#endif
}