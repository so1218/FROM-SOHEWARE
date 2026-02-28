#include "GrassField.h"
#include "ImGuiManager.h"
#include <random>

GrassField::GrassField(Engine* engine) : GameObject(engine)
{
    SetTag("GrassField");

    // システムの初期化 (引数のモデル名とテクスチャ名はご自身の環境に合わせてください)
    grassSystem_ = std::make_unique<GrassSystem>(engine_, "plane", "white1x1");

    binder_ = std::make_unique<PropertyBinder>(engine_, "GrassField");
}

void GrassField::Initialize()
{
    auto* grassMat = grassSystem_->GetMaterialData();

    // PropertyBinder にマテリアルデータをバインド
    binder_->Bind("WindSpeed", &grassMat->grassWindSpeed, 1.0f);
    binder_->Bind("WindAmplitude", &grassMat->grassWindAmplitude, 0.5f);
    binder_->Bind("NormalBlend", &grassMat->grassNormalBlend, 0.5f);
    binder_->Bind("Translucency", &grassMat->grassTranslucency, 0.5f);
    binder_->Bind("RootAO", &grassMat->grassRootAO, 0.5f);
    binder_->Bind("AlphaCutoff", &grassMat->grassAlphaCutoff, 0.1f);

    // 初回の草生成
    GenerateGrass();
}

void GrassField::Update()
{
    // 配置した草自体は動かないのでUpdateは特に処理なし
}

void GrassField::Draw()
{
    grassSystem_->Draw();
}

void GrassField::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("草むら設定");

    // 配置に関する設定
    ImGui::Text("--- 配置設定 ---");
    bool needsRegenerate = false;

    // 数や範囲をスライダーでいじった瞬間に再生成フラグを立てる
    if (ImGui::DragInt("草の数", &grassCount_, 50, 1, 10000)) { needsRegenerate = true; }
    if (ImGui::DragFloat("配置範囲", &spreadRadius_, 0.5f, 1.0f, 100.0f)) { needsRegenerate = true; }

    if (ImGui::Button("ランダム再生成") || needsRegenerate)
    {
        GenerateGrass();
    }

    ImGui::Separator();
    ImGui::Text("--- 質感・風の設定 ---");

    // PropertyBinderによるパラメータ描画
    binder_->Draw("WindSpeed", "風の速さ");
    binder_->Draw("WindAmplitude", "風の強さ");
    binder_->Draw("NormalBlend", "法線の滑らかさ");
    binder_->Draw("Translucency", "透過光の強さ");
    binder_->Draw("RootAO", "根本の影の濃さ");
    binder_->Draw("AlphaCutoff", "アルファカットオフ");

    ImGui::End();
#endif
}

void GrassField::GenerateGrass()
{
    // 一度すべての草を消す
    grassSystem_->Clear();

    // 乱数生成器の準備
    std::mt19937 randEngine(std::random_device{}());
    std::uniform_real_distribution<float> posDist(-spreadRadius_, spreadRadius_);
    std::uniform_real_distribution<float> scaleDist(0.8f, 1.2f);     // 大きさのばらつき
    std::uniform_real_distribution<float> rotDist(0.0f, 6.283185f);  // Y軸回転 (0 〜 2π)

    // このGameObjectの座標を基準に配置
    Vector3 basePos = transform_.translation_;

    for (int i = 0; i < grassCount_; ++i)
    {
        Vector3 pos = basePos;
        pos.x += posDist(randEngine);
        pos.z += posDist(randEngine); // Y（高さ）は変えず、XとZにばらまく

        Vector3 rot = { 0.0f, rotDist(randEngine), 0.0f };
        Vector3 scale = { scaleDist(randEngine), scaleDist(randEngine), scaleDist(randEngine) };

        // システムに草を追加
        grassSystem_->AddGrass(pos, rot, scale);
    }
}