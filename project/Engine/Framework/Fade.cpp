#include "pch.h"
#include "Fade.h"
#include "GlobalVariables.h"
#include "ImGuiManager.h"
#include "TimeManager.h"

namespace FE
{

Fade::Fade(Engine* engine)
{
    engine_ = engine;

    sprite_ = std::make_unique<Sprite>(engine_);
}

void Fade::Initialize()
{
    spriteSize_ = { (float)Engine::GetClientWidth(),(float)Engine::GetClientHeight()};

    // 通常スプライト
    sprite_->SetPosition(spritePos_);
    sprite_->SetSize(spriteSize_);
    sprite_->SetColor(0x000000FF);
    sprite_->SetLayerOrder(9999);

    sprite_->SetIsVisible(false);
    sprite_->SetDissolveTexture("noise_39");

    // グローバル変数登録
    binder_ = std::make_unique<PropertyBinder>(engine_, "Fade");

    binder_->Bind("duration_", &duration_, 1.0f);
    binder_->Bind("Enable Alpha Fade", &useAlphaFade_, true);
    binder_->Bind("Enable Dissolve", &useDissolve_, false);
    binder_->Bind("Dissolve Edge Width", &edgeWidth_, 0.04f);
    binder_->Bind("Dissolve Intensity", &edgeIntensity_, 2.0f);
    binder_->BindColor("Dissolve Color", &edgeColor_, { 1.0f, 1.0f, 1.0f });
}

void Fade::Update()
{
    // フェードなしの状態なら非表示にして終了
    if (status_ == Status::None)
    {
        sprite_->SetIsVisible(false);
        return;
    }

    // フェード中なら表示ON
    sprite_->SetIsVisible(true);

    // 時間経過の処理
    counter_ += TimeManager::GetInstance()->GetDeltaTime();
    if (counter_ >= duration_)
    {
        counter_ = duration_;
    }

    // 進行度t
    float t = std::clamp(counter_ / duration_, 0.0f, 1.0f);

    // 透明度フェード
    float alpha = 1.0f;

    if (useAlphaFade_)
    {
        if (status_ == Status::FadeIn)
        {
            // フェードイン
            alpha = 1.0f - t;
        }
        else
        {
            // フェードアウト
            alpha = t;
        }
    }

    // 計算したアルファ値を色に反映
    sprite_->SetColor(Vector4{ 0.0f, 0.0f, 0.0f, alpha });

    // ディゾルブの計算
    if (useDissolve_)
    {
        // Dissolveを有効化
        sprite_->SetEnableDissolve(true);

        auto* material = sprite_->GetMaterial();
        if (material)
        {
            material->edgeWidth = edgeWidth_;
            material->edgeIntensity = edgeIntensity_;
            material->edgeColor = edgeColor_;

            // Thresholdの計算
            if (status_ == Status::FadeIn)
            {
                // フェードイン
                material->dissolveThreshold = t;
            }
            else
            {
                // フェードアウト
                material->dissolveThreshold = 1.0f - t;
            }
        }
    }
    else
    {
        // ディゾルブを使わない場合は機能をOFF
        sprite_->SetEnableDissolve(false);
    }
}

void Fade::Draw()
{
    if (status_ == Status::None)
    {
        return;
    }
    sprite_->Draw();
}

void Fade::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("フェード設定");

    if (ImGui::CollapsingHeader("基本設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("duration_", "フェード時間(秒)");
        binder_->Draw("Enable Alpha Fade", "通常フェード有効化");
    }

    if (ImGui::CollapsingHeader("ディゾルブ設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("Enable Dissolve", "ディゾルブ有効化");

        if (useDissolve_)
        {
            binder_->Draw("Dissolve Edge Width", "エッジの幅");
            binder_->Draw("Dissolve Intensity", "エッジの発光強度");
            binder_->Draw("Dissolve Color", "エッジの色");
        }
    }

    ImGui::Separator();

    ImGui::End();
#endif
}

void Fade::Start(Status status, float duration)
{
    status_ = status;
    duration_ = duration;
    counter_ = 0.0f;

    // 開始時に表示ON
    sprite_->SetIsVisible(true);
}

void Fade::Stop()
{
    status_ = Status::None;
    sprite_->SetIsVisible(false);
}

bool Fade::IsFinished() const
{
    // フェード状態による分岐
    switch (status_)
    {
    case Status::FadeIn:
    case Status::FadeOut:
        if (counter_ >= duration_)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    return true;
}

}