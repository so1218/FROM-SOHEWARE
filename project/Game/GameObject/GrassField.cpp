#include "pch.h"
#include "GrassField.h"
#include "ImGuiManager.h"
#include "Player.h"

using namespace FE;

GrassField::GrassField(FE::Engine* engine, Player* player) : FE::GameObject()
{
    engine_ = engine;

    grassSystem_ = std::make_unique<FE::GrassSystem>(engine_, "noise_39");
    binder_ = std::make_unique<FE::PropertyBinder>(engine_, "GrassField");

    player_ = player;
}

void GrassField::Initialize()
{
    auto* grassMat = grassSystem_->GetMaterialData();
    auto* cullingData = grassSystem_->GetCullingData();

    // ==========================================
     // 配置設定
     // ==========================================
    binder_->Bind("GrassCount", &grassCount_, 3000);
    binder_->Bind("GridSpacing", &gridSpacing_, 0.5f, 0.01f, 0.1f, 2.0f);
    binder_->Bind("SpreadRadius", &spreadRadius_, 20.0f);
    binder_->Bind("Position", &transform_.translation_, { 0.0f, 0.0f, 0.0f });
    binder_->Bind("BaseScale", &baseScale_, 1.0f);
    binder_->Bind("BaseHeight", &baseHeight_, 1.0f);
    binder_->Bind("BaseWidth", &baseWidth_, 1.0f);
    binder_->BindColor("BaseGrassColor", &baseGrassColor_, { 0.4f, 0.7f, 0.2f, 1.0f }); // インスタンスごとの基本色

    // ==========================================
    // 色とライティング (構造体に合わせた変更・追加)
    // ==========================================
    binder_->BindColor("RootColor", &grassMat->rootColor, { 0.1f, 0.2f, 0.05f }); // 根本の暗い色
    binder_->Bind("GrassRootAO", &grassMat->grassRootAO, 0.3f, 0.05f, 0.0f, 1.0f);

    binder_->BindColor("TipColor", &grassMat->tipColor, { 1.0f, 1.0f, 1.0f }); // 先端の色(BaseGrassColorと乗算されるため白ベース推奨)
    binder_->Bind("GrassNormalBlend", &grassMat->grassNormalBlend, 0.7f, 0.05f, 0.0f, 1.0f); // 0.6~0.8推奨

    binder_->BindColor("SSSColor", &grassMat->sssColor, { 0.7f, 0.9f, 0.3f }); // 太陽に透けた時の色
    binder_->Bind("SSSStrength", &grassMat->sssStrength, 1.5f, 0.1f, 0.0f, 5.0f);

    binder_->Bind("ColorVariation", &grassMat->colorVariation, 0.5f, 0.05f, 0.0f, 1.0f);

    binder_->Bind("SpecularStrength", &grassMat->specularStrength, 0.2f, 0.01f, 0.0f, 1.0f); // 0.1~0.3推奨
    binder_->Bind("SpecularShininess", &grassMat->specularShininess, 40.0f, 1.0f, 10.0f, 200.0f);
    binder_->Bind("Wetness", &grassMat->wetness, 0.0f, 0.05f, 0.0f, 1.0f);

    // ==========================================
    // 風の挙動 (構造体に合わせた変更・追加)
    // ==========================================
    binder_->Bind("WindDir", &grassMat->windDir, { 1.0f, 0.8f });
    binder_->Bind("WindSpeed", &grassMat->windSpeed, 1.5f, 0.1f, 0.0f, 10.0f);
    binder_->Bind("BaseWindStrength", &grassMat->baseWindStrength, 0.3f, 0.05f, 0.0f, 2.0f);

    binder_->Bind("GustScale", &grassMat->gustScale, 0.03f, 0.005f, 0.001f, 0.5f);
    binder_->Bind("GustStrength", &grassMat->gustStrength, 1.2f, 0.1f, 0.0f, 5.0f);
    binder_->Bind("WindFlattenStrength", &grassMat->windFlattenStrength, 1.0f, 0.05f, 0.0f, 3.0f); 
    binder_->Bind("FlutterAmount", &grassMat->flutterAmount, 0.15f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("WindHighlightStrength", &grassMat->windHighlightStrength, 0.4f, 0.05f, 0.0f, 1.0f);

    // ==========================================
    // インタラクション (プレイヤー干渉)
    // ==========================================
    binder_->Bind("InteractRadius", &grassMat->interactRadius, 1.2f, 0.1f, 0.1f, 5.0f);
    binder_->Bind("InteractStrength", &grassMat->interactStrength, 1.0f, 0.1f, 0.0f, 3.0f);

    // ==========================================
    // 影
    // ==========================================
    binder_->Bind("ShadowDensity", &grassMat->shadowDensity, 0.8f, 0.05f, 0.0f, 1.0f);
    binder_->Bind("ShadowBias", &grassMat->shadowBias, 0.005f, 0.001f, 0.0f, 0.05f);
    binder_->Bind("ShadowNormalBias", &grassMat->shadowNormalBias, 0.02f, 0.001f, 0.0f, 0.1f);

    // ==========================================
 // カリングとLOD
 // ==========================================
    binder_->Bind("MaxDrawDistance", &cullingData->maxDrawDistance, 150.0f, 1.0f, 10.0f, 1000.0f);
    binder_->Bind("ThinStartDistance", &cullingData->thinStartDistance, 50.0f, 1.0f, 10.0f, 500.0f);
    binder_->Bind("MaxThinningRate", &cullingData->maxThinningRate, 0.8f, 0.05f, 0.0f, 0.99f);
    binder_->Bind("MaxWidthMultiplier", &cullingData->maxWidthMultiplier, 2.5f, 0.1f, 1.0f, 5.0f);

    binder_->Bind("LodDistance1", &cullingData->lodDistance1, 20.0f, 1.0f, 5.0f, 100.0f);
    binder_->Bind("LodDistance2", &cullingData->lodDistance2, 50.0f, 1.0f, 10.0f, 200.0f);

    // 初期状態を記憶
    prevGrassCount_ = grassCount_;
    prevGridSpacing_ = gridSpacing_;
    prevSpreadRadius_ = spreadRadius_;
    prevPosition_ = transform_.translation_;
    prevBaseScale_ = baseScale_;
    prevBaseGrassColor_ = baseGrassColor_;
    prevBaseWidth_ = baseWidth_;

    // 初回の草生成
    GenerateGrass();
}

void GrassField::Update()
{
    // 座標やスケール、太さ、高さ、色が変更されたら草を再配置
    if (transform_.translation_.x != prevPosition_.x ||
        transform_.translation_.y != prevPosition_.y ||
        transform_.translation_.z != prevPosition_.z ||
        baseScale_ != prevBaseScale_ ||
        grassCount_ != prevGrassCount_ ||
        spreadRadius_ != prevSpreadRadius_ ||
        prevGridSpacing_ != gridSpacing_ ||
        baseWidth_ != prevBaseWidth_ ||               
        baseHeight_ != prevBaseHeight_ ||             
        baseGrassColor_.x != prevBaseGrassColor_.x || 
        baseGrassColor_.y != prevBaseGrassColor_.y || 
        baseGrassColor_.z != prevBaseGrassColor_.z)   
    {
        GenerateGrass();

        // 記憶を更新
        prevGrassCount_ = grassCount_;
        prevGridSpacing_ = gridSpacing_;
        prevSpreadRadius_ = spreadRadius_;
        prevPosition_ = transform_.translation_;
        prevBaseScale_ = baseScale_;

        prevBaseWidth_ = baseWidth_;                 
        prevBaseHeight_ = baseHeight_;               
        prevBaseGrassColor_ = baseGrassColor_;       
    }

    if (player_)
    {
        grassSystem_->GetMaterialData()->playerPos = player_->animationModel_->GetTransform().translation_;
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

    if (ImGui::CollapsingHeader("配置設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Position", "中心座標");
        binder_->Draw("BaseScale", "全体の大きさ");
        binder_->Draw("BaseHeight", "草の高さ");
        binder_->Draw("BaseWidth", "草の太さ");
        binder_->Draw("GrassCount", "草の数");
        binder_->Draw("GridSpacing", "草の間隔 (小さいほど高密度)");
        binder_->Draw("SpreadRadius", "配置範囲");

        if (ImGui::Button("ランダム再生成"))
        {
            GenerateGrass();
        }
    }

    if (ImGui::CollapsingHeader("質感・ライティング"))
    {
        // 色設定
        binder_->Draw("BaseGrassColor", "草の基本色 (インスタンス全体)");
        binder_->Draw("ColorVariation", "草原全体の色ムラ");

        ImGui::Separator();
        binder_->Draw("RootColor", "根本の色");
        binder_->Draw("GrassRootAO", "根本のAO(暗さ)");
        binder_->Draw("TipColor", "先端の乗算色");

        ImGui::Separator();
        // ライティング
        binder_->Draw("GrassNormalBlend", "法線の上向きブレンド (最重要)");
        binder_->Draw("SSSColor", "透過光(SSS)の色");
        binder_->Draw("SSSStrength", "透過光(SSS)の強さ");

        ImGui::Separator();
        // スペキュラ・濡れ
        binder_->Draw("SpecularStrength", "ハイライトの基本強度");
        binder_->Draw("SpecularShininess", "ハイライトの鋭さ");
        binder_->Draw("Wetness", "濡れ具合");
    }

    if (ImGui::CollapsingHeader("風の挙動"))
    {
        binder_->Draw("WindDir", "風向き(X, Z)");
        binder_->Draw("WindSpeed", "風の移動速度");
        binder_->Draw("BaseWindStrength", "常時吹くそよ風の強さ");

        ImGui::Separator();
        binder_->Draw("GustScale", "突風ノイズのスケール");
        binder_->Draw("GustStrength", "突風の強さ(水平方向)");
        binder_->Draw("WindFlattenStrength", "突風時の押し潰し(下方向)");
        binder_->Draw("GustThreshold", "突風の発生しきい値(0無風領域)");
        binder_->Draw("GustContrast", "突風のメリハリ(波の鋭さ)");
        binder_->Draw("FlutterAmount", "葉先の震えの強さ");
        binder_->Draw("WindHighlightStrength", "風による光沢変化(シルバーライニング)");
    }

    if (ImGui::CollapsingHeader("インタラクション"))
    {
        binder_->Draw("InteractRadius", "かき分ける半径");
        binder_->Draw("InteractStrength", "押し倒す強さ");
    }

    if (ImGui::CollapsingHeader("影の設定"))
    {
        binder_->Draw("ShadowDensity", "影の濃さ");
        binder_->Draw("ShadowBias", "深度バイアス");
        binder_->Draw("ShadowNormalBias", "法線バイアス");
    }

    if (ImGui::CollapsingHeader("カリング・LOD設定"))
    {
        binder_->Draw("MaxDrawDistance", "最大描画距離 (これより遠い草は消去)");

        ImGui::Separator();
        binder_->Draw("ThinStartDistance", "間引き開始距離");
        binder_->Draw("MaxThinningRate", "最大間引き率 (1.0に近いほど消える)");
        binder_->Draw("MaxWidthMultiplier", "間引き時の太さ補正倍率");

        ImGui::Separator();
        binder_->Draw("LodDistance1", "LOD1の距離 (高ポリ境界)");
        binder_->Draw("LodDistance2", "LOD2の距離 (中ポリ境界)");
    }


    ImGui::End();
#endif
}

void GrassField::GenerateGrass()
{
    // ★重要: 古い草を消して更新フラグを立てる
    grassSystem_->Clear();

    std::mt19937 randEngine(std::random_device{}());
    std::uniform_real_distribution<float> scaleDist(0.8f, 1.2f);
    std::uniform_real_distribution<float> rotDist(0.0f, 6.283185f); // 0 ~ 2π

    FE::Vector3 basePos = transform_.translation_;

    // ★ ローカル変数の float gridSpacing = 0.5f; を削除し、メンバ変数 gridSpacing_ を使用する
    float jitterAmount = gridSpacing_ * 0.45f;
    std::uniform_real_distribution<float> jitterDist(-jitterAmount, jitterAmount);

    // spreadRadius_ を一辺の半分とした正方形の範囲にグリッド状に配置
    for (float z = -spreadRadius_; z <= spreadRadius_; z += gridSpacing_)
    {
        for (float x = -spreadRadius_; x <= spreadRadius_; x += gridSpacing_)
        {
            FE::Vector3 pos = basePos;
            // グリッドの基本位置 + ランダムなズレ
            pos.x += x + jitterDist(randEngine);
            pos.z += z + jitterDist(randEngine);

            if (terrain_) {
                pos.y = terrain_->GetHeight(pos.x, pos.z);
            }
            else {
                pos.y = 0.0f;
            }

            float rotY = rotDist(randEngine);
            float finalScale = scaleDist(randEngine) * baseScale_;
            float height = finalScale * baseHeight_;
            float width = finalScale * baseWidth_;

            grassSystem_->AddGrass(pos, height, rotY, width, baseGrassColor_);
        }
    }
}