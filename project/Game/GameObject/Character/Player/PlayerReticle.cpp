#include "pch.h"
#include "PlayerReticle.h"
#include "Engine.h"
#include "TimeManager.h"

using namespace FE;

PlayerReticle::PlayerReticle(Engine* engine)
    : engine_(engine)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "PlayerReticle");
    sprite_ = std::make_unique<Sprite>(engine_);
}

void PlayerReticle::Initialize()
{
    sprite_->SetTexture("white1x1");
    sprite_->SetAnchorPoint({ 0.5f, 0.5f });
    sprite_->SetIsVisible(true);

    // Binderへパラメータ登録
    binder_->Bind("LineThickness", &config_.lineThickness, 0.5f, 0.1f, 1.0f, 10.0f);
    binder_->Bind("LineLength", &config_.lineLength, 1.0f, 0.1f, 2.0f, 50.0f);
    binder_->Bind("MaxGap", &config_.maxGap, 1.0f, 0.1f, 10.0f, 100.0f);
    binder_->Bind("MinGap", &config_.minGap, 0.5f, 0.1f, 0.0f, 30.0f);
    binder_->Bind("CenterDotSize", &config_.centerDotSize, 0.5f, 0.1f, 1.0f, 20.0f);
    binder_->Bind("FocusTime", &config_.focusTime, 0.05f, 0.1f, 0.1f, 3.0f);
    binder_->Bind("ExpandSpeed", &config_.expandSpeed, 0.5f, 0.1f, 1.0f, 20.0f);
    binder_->Bind("Threshold", &config_.threshold, 0.05f, 0.1f, 0.5f, 0.95f);
    binder_->Bind("MidRatio", &config_.midRatio, 0.05f, 0.1f, 0.1f, 0.8f);
    binder_->Bind("FinalExponent", &config_.finalExponent, 0.5f, 0.1f, 1.0f, 8.0f);
}

void PlayerReticle::Update(bool isMoving, bool isAiming)
{
    if (!isAiming)
    {
        Reset();
        return;
    }

    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // エイム中のアルファ値フェードイン
    reticleAlpha_ = Math::Lerp(reticleAlpha_, 1.0f, 15.0f * deltaTime);

    if (!isMoving)
    {
        focusTimer_ += deltaTime;
    }
    else
    {
        focusTimer_ -= deltaTime * config_.expandSpeed;
    }

    focusTimer_ = std::clamp(focusTimer_, 0.0f, config_.focusTime);
    focusRatio_ = focusTimer_ / config_.focusTime;
}

void PlayerReticle::Reset()
{
    focusTimer_ = 0.0f;
    focusRatio_ = 0.0f;
    reticleAlpha_ = 0.0f;
}

void PlayerReticle::OnShootRecoil()
{
    focusTimer_ *= 0.2f;
}

void PlayerReticle::Draw()
{
    if (!sprite_ || reticleAlpha_ <= 0.001f) return;

    float centerX = static_cast<float>(Engine::GetClientWidth()) * 0.5f;
    float centerY = static_cast<float>(Engine::GetClientHeight()) * 0.5f;

    float easedRatio = 0.0f;
    float t = std::clamp(focusRatio_, 0.0f, 1.0f);

    if (t <= config_.threshold)
    {
        // threshold まで縮める
        float normalizedT = t / config_.threshold;
        easedRatio = config_.midRatio * normalizedT;
    }
    else
    {
		// threshold 以降は加速して収束
        float normalizedT = (t - config_.threshold) / (1.0f - config_.threshold);
        easedRatio = config_.midRatio + (1.0f - config_.midRatio) * std::pow(normalizedT, config_.finalExponent);
    }

    float currentGap = Math::Lerp(config_.maxGap, config_.minGap, easedRatio);
    bool isFullyFocused = (focusRatio_ >= 0.98f);

    float thickness = config_.lineThickness;
    float length = config_.lineLength;

    // 4本線の描画
    // 上
    sprite_->SetPosition({ centerX, centerY - currentGap - length * 0.5f });
    sprite_->SetSize({ thickness, length });
    sprite_->Draw();

    // 下
    sprite_->SetPosition({ centerX, centerY + currentGap + length * 0.5f });
    sprite_->SetSize({ thickness, length });
    sprite_->Draw();

    // 左
    sprite_->SetPosition({ centerX - currentGap - length * 0.5f, centerY });
    sprite_->SetSize({ length, thickness });
    sprite_->Draw();

    // 右
    sprite_->SetPosition({ centerX + currentGap + length * 0.5f, centerY });
    sprite_->SetSize({ length, thickness });
    sprite_->Draw();

    // 中心ドット
    if (isFullyFocused && config_.centerDotSize > 0.0f)
    {
        float dotSize = config_.centerDotSize;
        sprite_->SetPosition({ centerX, centerY });
        sprite_->SetSize({ dotSize, dotSize });
        sprite_->Draw();
    }
}

void PlayerReticle::DebugDraw()
{
#ifdef ENABLE_IMGUI
    if (ImGui::CollapsingHeader("レティクル設定"))
    {
        ImGui::Indent();

        binder_->Draw("LineThickness", "線の太さ");
        binder_->Draw("LineLength", "線の長さ");
        binder_->Draw("MaxGap", "最大広がり距離");
        binder_->Draw("MinGap", "最小収束距離");
        binder_->Draw("CenterDotSize", "完全収束時の中心ドットサイズ");

        ImGui::Spacing();
        binder_->Draw("FocusTime", "フォーカス完了時間");
        binder_->Draw("ExpandSpeed", "移動時拡散スピード");

        ImGui::Separator();
        ImGui::Text("カーブ調整");
        binder_->Draw("Threshold", "加速開始タイミング");
        binder_->Draw("MidRatio", "加速開始時点の収束率");
        binder_->Draw("FinalExponent", "ラストスパートの急速度");

        ImGui::Unindent();
    }
#endif
}