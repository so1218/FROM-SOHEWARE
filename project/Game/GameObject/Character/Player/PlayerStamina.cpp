#include "pch.h"
#include "PlayerStamina.h"
#include "TimeManager.h"

using namespace FS;

PlayerStamina::PlayerStamina(Engine* engine)
{
    gaugeBack_ = std::make_unique<Sprite>(engine);
    gaugeFill_ = std::make_unique<Sprite>(engine);
}

void PlayerStamina::BindProperties(PropertyBinder& binder)
{
    binder.Bind("Stamina_Max", &maxStamina_, 100.0f, 1.0f, 1.0f, 1000.0f);
    binder.Bind("Stamina_RegenPerSecond", &regenPerSecond_, 30.0f, 0.5f, 0.0f, 500.0f);
    binder.Bind("Stamina_RegenDelay", &regenDelay_, 0.8f, 0.01f, 0.0f, 5.0f);
    binder.Bind("Stamina_ExhaustedRecover", &exhaustedRecoverAmount_, 40.0f, 1.0f, 0.0f, 1000.0f);

    binder.Bind("Stamina_GaugeOffset", &gaugeOffset_, { 0.0f, -60.0f }, 1.0f);
    binder.Bind("Stamina_GaugeSize", &gaugeSize_, { 240.0f, 6.0f }, 1.0f, 1.0f, 1280.0f);
    binder.BindColor("Stamina_GaugeColor", &gaugeColor_, { 1.0f, 1.0f, 1.0f, 0.9f });
    binder.BindColor("Stamina_ExhaustedColor", &exhaustedColor_, { 0.9f, 0.25f, 0.2f, 0.9f });

    currentStamina_ = maxStamina_;
    isExhausted_ = false;
}

void PlayerStamina::DrawProperties(PropertyBinder& binder)
{
#ifdef ENABLE_IMGUI
    ImGui::Text("現在: %.1f / %.1f %s", currentStamina_, maxStamina_, isExhausted_ ? "(息切れ)" : "");
    binder.Draw("Stamina_Max", "最大値");
    binder.Draw("Stamina_RegenPerSecond", "1 秒あたりの回復量");
    binder.Draw("Stamina_RegenDelay", "回復が始まるまでの時間 (秒)");
    binder.Draw("Stamina_ExhaustedRecover", "息切れから戻る量");
    binder.Draw("Stamina_GaugeOffset", "ゲージの位置 (画面下中央から)");
    binder.Draw("Stamina_GaugeSize", "ゲージの大きさ");
    binder.Draw("Stamina_GaugeColor", "ゲージの色");
    binder.Draw("Stamina_ExhaustedColor", "息切れ中の色");
#endif
}

void PlayerStamina::Update()
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    timeSinceUsed_ += deltaTime;

    if (timeSinceUsed_ >= regenDelay_)
    {
        currentStamina_ = std::min(currentStamina_ + regenPerSecond_ * deltaTime, maxStamina_);
    }

    if (isExhausted_ && currentStamina_ >= exhaustedRecoverAmount_)
    {
        isExhausted_ = false;
    }
}

bool PlayerStamina::TryConsume(float amount)
{
    if (currentStamina_ < amount)
    {
        return false;
    }

    currentStamina_ -= amount;
    timeSinceUsed_ = 0.0f;
    if (currentStamina_ <= 0.0f)
    {
        isExhausted_ = true;
    }
    return true;
}

void PlayerStamina::Drain(float amountPerSecond)
{
    currentStamina_ -= amountPerSecond * TimeManager::GetInstance()->GetDeltaTime();
    timeSinceUsed_ = 0.0f;
    if (currentStamina_ <= 0.0f)
    {
        currentStamina_ = 0.0f;
        isExhausted_ = true;
    }
}

void PlayerStamina::DrawGauge()
{
    // 満タンのときは出さない
    if (currentStamina_ >= maxStamina_)
    {
        return;
    }

    const Vector2 gaugeLeft(gaugeOffset_.x - gaugeSize_.x * 0.5f, gaugeOffset_.y);
    const float staminaRatio = currentStamina_ / maxStamina_;

    gaugeBack_->SetScreenAnchor({ 0.5f, 1.0f });
    gaugeBack_->SetAnchorPoint({ 0.0f, 0.5f });
    gaugeBack_->SetPosition(gaugeLeft);
    gaugeBack_->SetSize(gaugeSize_);
    gaugeBack_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.5f));
    gaugeBack_->SetLayerOrder(10);
    gaugeBack_->Draw();

    gaugeFill_->SetScreenAnchor({ 0.5f, 1.0f });
    gaugeFill_->SetAnchorPoint({ 0.0f, 0.5f });
    gaugeFill_->SetPosition(gaugeLeft);
    gaugeFill_->SetSize({ gaugeSize_.x * staminaRatio, gaugeSize_.y });
    gaugeFill_->SetColor(isExhausted_ ? exhaustedColor_ : gaugeColor_);
    gaugeFill_->SetLayerOrder(11);
    gaugeFill_->Draw();
}
