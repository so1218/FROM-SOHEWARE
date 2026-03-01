#include "GrassField.h"
#include "ImGuiManager.h"
#include "Player.h"
#include <random>

GrassField::GrassField(Engine* engine, Player* player) : GameObject(engine)

{
    SetTag("GrassField");

    grassSystem_ = std::make_unique<GrassSystem>(engine_, "grass", "white1x1");

    binder_ = std::make_unique<PropertyBinder>(engine_, "GrassField");

    player_ = player;
}

void GrassField::Initialize()
{
    auto* grassMat = grassSystem_->GetMaterialData();

    binder_->Bind("GrassCount", &grassCount_, 50);
    binder_->Bind("SpreadRadius", &spreadRadius_, 10.0f);
    binder_->Bind("Position", &transform_.translation_, { 0.0f,0.0f,0.0f });
    binder_->Bind("BaseScale", &baseScale_, 1.0f);

    binder_->BindColor("Color", &grassMat->color, { 1.0f,1.0f,1.0f,1.0f });
    binder_->Bind("ShadowDensity", &grassMat->shadowDensity, 0.5f);
    binder_->Bind("Wetness", &grassMat->wetness, 0.0f);
    binder_->Bind("Roughness", &grassMat->roughness, 0.8f);

    binder_->Bind("WindSpeed", &grassMat->grassWindSpeed, 1.0f);
    binder_->Bind("WindAmplitude", &grassMat->grassWindAmplitude, 0.5f);
    binder_->Bind("NormalBlend", &grassMat->grassNormalBlend, 0.5f);
    binder_->Bind("Translucency", &grassMat->grassTranslucency, 0.5f);
    binder_->Bind("RootAO", &grassMat->grassRootAO, 0.5f);
    binder_->Bind("AlphaCutoff", &grassMat->grassAlphaCutoff, 0.1f);
    binder_->Bind("InteractRadius", &grassMat->interactRadius, 1.5f);
    binder_->Bind("InteractStrength", &grassMat->interactStrength, 1.0f);

    // 初期状態を記憶
    prevGrassCount_ = grassCount_;     
    prevSpreadRadius_ = spreadRadius_;
    prevPosition_ = transform_.translation_;
    prevBaseScale_ = baseScale_;

    // 初回の草生成
    GenerateGrass();
}

void GrassField::Update()
{
    // 座標やスケールが変更されたら草を再配置
    if (transform_.translation_.x != prevPosition_.x ||
        transform_.translation_.y != prevPosition_.y ||
        transform_.translation_.z != prevPosition_.z ||
        baseScale_ != prevBaseScale_ ||
        grassCount_ != prevGrassCount_ ||    
        spreadRadius_ != prevSpreadRadius_)
    {
        GenerateGrass();

        // 記憶を更新
        prevGrassCount_ = grassCount_;       
        prevSpreadRadius_ = spreadRadius_;
        prevPosition_ = transform_.translation_;
        prevBaseScale_ = baseScale_;
    }

    if (player_)
    {
        grassSystem_->GetMaterialData()->playerPos = player_->animationPlayer_->GetTransform().translation_;
    }
}

void GrassField::Draw()
{
    grassSystem_->Draw();
}

void GrassField::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("草むら");

    ImGui::Text("配置設定");

    binder_->Draw("Position", "中心座標");
    binder_->Draw("BaseScale", "全体の大きさ");

    ImGui::DragInt("草の数", &grassCount_, 1, 1, 10000);
    ImGui::DragFloat("配置範囲", &spreadRadius_, 0.5f, 1.0f, 100.0f);

    if (ImGui::Button("ランダム再生成"))
    {
        GenerateGrass();
    }

    ImGui::Separator();
    ImGui::Text("質感・風の設定");

    binder_->Draw("Color", "草の色");
    binder_->Draw("ShadowDensity", "影の濃さ");
    binder_->Draw("Wetness", "濡れ度");
    binder_->Draw("Roughness", "ラフネス");

    binder_->Draw("WindSpeed", "風の速さ");
    binder_->Draw("WindAmplitude", "風の強さ");
    binder_->Draw("NormalBlend", "法線の滑らかさ");
    binder_->Draw("Translucency", "透過光の強さ");
    binder_->Draw("RootAO", "根本の影の濃さ");
    binder_->Draw("AlphaCutoff", "アルファカットオフ");

    ImGui::Separator();
    ImGui::Text("踏み込み設定");
    binder_->Draw("InteractRadius", "草が避ける範囲");
    binder_->Draw("InteractStrength", "草の倒れ具合");

    ImGui::End();
#endif
}

void GrassField::GenerateGrass()
{
    grassSystem_->Clear();

    std::mt19937 randEngine(std::random_device{}());
    std::uniform_real_distribution<float> posDist(-spreadRadius_, spreadRadius_);
    std::uniform_real_distribution<float> scaleDist(0.8f, 1.2f);
    std::uniform_real_distribution<float> rotDist(0.0f, 6.283185f);

    Vector3 basePos = transform_.translation_;

    for (int i = 0; i < grassCount_; ++i)
    {
        Vector3 pos = basePos;
        pos.x += posDist(randEngine);
        pos.z += posDist(randEngine);

        Vector3 rot = { 0.0f, rotDist(randEngine), 0.0f };

        float finalScale = scaleDist(randEngine) * baseScale_;
        Vector3 scale = { finalScale, finalScale, finalScale };

        grassSystem_->AddGrass(pos, rot, scale);
    }
}