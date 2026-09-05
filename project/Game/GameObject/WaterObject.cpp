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
    // 1. 個別の定数バッファを作成
    materialCBResource_ = BufferManager::CreateMappedConstantBuffer<WaterMaterialData>(
        engine_->GetGraphicsDevice()->GetDevice(),
        &mappedMaterialData_
    );


    // 2. プロパティのバインドを実行 (★ここで呼び出す必要があります)
    BindProperties();
}

void WaterObject::BindProperties()
{
    std::string childGroupName = "Water_" + std::to_string(id_);
    binder_ = std::make_unique<PropertyBinder>(engine_, parentGroupName_, childGroupName);

    // トランスフォーム
    binder_->Bind("Position", &model_->GetTransform().translation_, Vector3(0.0f, 0.0f, 0.0f));
    binder_->BindRotation("Rotation", &model_->GetTransform().rotation_, &model_->GetTransform().rotationQuaternion_, 0.01f);
    binder_->Bind("Scale", &model_->GetTransform().scale_, Vector3(10.0f, 1.0f, 10.0f));

    // テクスチャ
    binder_->BindTexture("NormalMap", &normalMapName_, &normalMapHandle_, "white1x1", TextureType::Normal);
    binder_->BindTexture("RippleTexture", &rippleTextureName_, &rippleTextureHandle_, "white1x1", TextureType::Normal);
    binder_->BindTexture("EnvMap", &envMapName_, &envMapSrvHandle_, "pureSky", TextureType::CubeMap);

    // 1. 風・波設定
    binder_->Bind("WindDirection", &materialData_.windDirection, Vector2(1.0f, 0.5f));
    binder_->Bind("BaseWaveLength", &materialData_.baseWaveLength, 15.0f, 0.1f, 0.5f, 100.0f);
    binder_->Bind("BaseAmplitude", &materialData_.baseAmplitude, 0.5f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("BaseSteepness", &materialData_.baseSteepness, 0.4f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("WaveSpeed", &materialData_.waveSpeed, 1.0f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("WavePersistence", &materialData_.wavePersistence, 0.5f, 0.01f, 0.1f, 1.0f);
    binder_->Bind("WaveLacunarity", &materialData_.waveLacunarity, 2.1f, 0.05f, 1.0f, 4.0f);
    binder_->Bind("WaveDirectionSpread", &materialData_.waveDirectionSpread, 0.3f, 0.01f, 0.0f, 1.57f);
    binder_->Bind("WaveChop", &materialData_.waveChop, 1.0f, 0.05f, 0.0f, 2.0f);
    binder_->Bind("NormalIntensity", &materialData_.normalIntensity, 0.7f, 0.05f, 0.0f, 1.0f); // ★
    binder_->Bind("WaveFoamThreshold", &materialData_.waveFoamThreshold, 0.25f, 0.01f, 0.01f, 1.0f); // ★

    // 2. カラー設定
    binder_->BindColor("ShallowColor", &materialData_.shallowColor, Vector4(0.1f, 0.6f, 0.8f, 0.8f));
    binder_->BindColor("DeepColor", &materialData_.deepColor, Vector4(0.0f, 0.1f, 0.3f, 1.0f));
    binder_->BindColor("ScatterColor", &materialData_.scatterColor, Vector4(0.0f, 0.4f, 0.3f, 1.0f));
    binder_->BindColor("FoamColor", &materialData_.foamColor, Vector4(0.95f, 0.98f, 1.0f, 0.9f));

    // 3. ライティング・反射・屈折
    binder_->Bind("Absorption", &materialData_.absorption, 0.5f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("RefractionAmount", &materialData_.refractionAmount, 0.05f, 0.001f, 0.0f, 0.5f);
    binder_->Bind("ChromaticAberration", &materialData_.chromaticAberration, 0.5f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("WaveTiling", &materialData_.waveTiling, Vector2(0.1f, 0.1f));
    binder_->Bind("Roughness", &materialData_.roughness, 0.1f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("SpecularIntensity", &materialData_.specularIntensity, 1.0f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("EnvReflectionIntensity", &materialData_.envReflectionIntensity, 1.0f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("SSRIntensity", &materialData_.ssrIntensity, 1.0f, 0.05f, 0.0f, 2.0f);
    binder_->Bind("SSRThickness", &materialData_.ssrThickness, 0.5f, 0.01f, 0.05f, 5.0f);
    binder_->Bind("SSRStepSize", &materialData_.ssrStepSize, 0.15f, 0.01f, 0.01f, 1.0f);
    binder_->Bind("SSRMaxDistance", &materialData_.ssrMaxDistance, 50.0f, 1.0f, 5.0f, 200.0f);

    // 4. コースティクス・泡
    binder_->Bind("CausticsScale", &materialData_.causticsScale, 1.0f, 0.05f, 0.1f, 10.0f);
    binder_->Bind("CausticsIntensity", &materialData_.causticsIntensity, 0.5f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("CausticsFadeDepth", &materialData_.causticsFadeDepth, 3.0f, 0.1f, 0.1f, 20.0f);
    binder_->Bind("CausticsSpeed", &materialData_.causticsSpeed, 0.05f, 0.01f, 0.0f, 0.5f);
    binder_->Bind("CausticsDistortion", &materialData_.causticsDistortion, 0.5f, 0.05f, 0.0f, 2.0f);
    binder_->Bind("FoamScale", &materialData_.foamScale, 0.5f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("FoamThreshold", &materialData_.foamThreshold, 0.3f, 0.01f, 0.01f, 2.0f);
    binder_->Bind("FoamIntensity", &materialData_.foamIntensity, 1.0f, 0.05f, 0.0f, 5.0f);

    // 5. 雨・波紋
    binder_->Bind("RainIntensity", &materialData_.rainIntensity, 0.0f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("RippleScale", &materialData_.rippleScale, 5.0f, 0.1f, 0.1f, 50.0f);
    binder_->Bind("RippleSpeed", &materialData_.rippleSpeed, 2.0f, 0.1f, 0.0f, 10.0f);
    binder_->Bind("RippleStrength", &materialData_.rippleStrength, 0.2f, 0.01f, 0.0f, 1.0f);
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


        if (ImGui::TreeNode("波・風・法線設定"))
        {
            binder_->Draw("WindDirection", "風向き");
            binder_->Draw("BaseWaveLength", "基本波長 (m)");
            binder_->Draw("BaseAmplitude", "基本振幅");
            binder_->Draw("BaseSteepness", "波の鋭さ");
            binder_->Draw("WaveSpeed", "進行速度");
            binder_->Draw("WavePersistence", "小波減衰率");
            binder_->Draw("WaveLacunarity", "小波周波数倍率");
            binder_->Draw("WaveDirectionSpread", "子波拡散角度");
            binder_->Draw("WaveChop", "水平引き寄せ強度");
            binder_->Draw("NormalIntensity", "法線マップ適用強度");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("カラー & 散乱設定"))
        {
            binder_->Draw("ShallowColor", "浅瀬の色");
            binder_->Draw("DeepColor", "深い場所の色");
            binder_->Draw("ScatterColor", "水中散乱光の色");
            binder_->Draw("FoamColor", "泡の色・透明度");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("ライティング・反射・屈折"))
        {
            binder_->Draw("Absorption", "吸光度");
            binder_->Draw("RefractionAmount", "屈折歪み強度");
            binder_->Draw("ChromaticAberration", "色収差強度");
            binder_->Draw("WaveTiling", "法線タイリング");
            binder_->Draw("Roughness", "ラフネス");
            binder_->Draw("SpecularIntensity", "ハイライト強度");
            binder_->Draw("EnvReflectionIntensity", "環境反射強度");
            binder_->Draw("SSRIntensity", "SSR反射強度");
            binder_->Draw("SSRThickness", "SSR判定厚み");
            binder_->Draw("SSRStepSize", "SSRレイ初期ステップ幅");
            binder_->Draw("SSRMaxDistance", "SSR最大描画距離");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("コースティクス & 泡"))
        {
            binder_->Draw("CausticsScale", "コースティクス・スケール");
            binder_->Draw("CausticsIntensity", "コースティクス・強度");
            binder_->Draw("CausticsFadeDepth", "コースティクス消滅深度");
            binder_->Draw("CausticsSpeed", "コースティクス揺らぎ速度");
            binder_->Draw("CausticsDistortion", "コースティクス屈折歪み");
            binder_->Draw("FoamScale", "泡ノイズ・スケール");
            binder_->Draw("FoamThreshold", "岸辺の泡の範囲");
            binder_->Draw("WaveFoamThreshold", "波頭の泡の発生しきい値");
            binder_->Draw("FoamIntensity", "泡の濃さ");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("雨 & 波紋エフェクト"))
        {
            binder_->Draw("RainIntensity", "雨の強度");
            binder_->Draw("RippleScale", "波紋密度");
            binder_->Draw("RippleSpeed", "波紋速度");
            binder_->Draw("RippleStrength", "波紋法線強度");
            ImGui::TreePop();
        }

        ImGui::Unindent();
        ImGui::Spacing();
    }

    ImGui::PopID();
#endif
}