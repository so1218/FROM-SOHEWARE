#include "pch.h"
#include "PlayerHealth.h"
#include "TimeManager.h"
#include "Engine.h"

using namespace FS;

PlayerHealth::PlayerHealth(Engine* engine)
{
    gaugeBack_ = std::make_unique<Sprite>(engine);
    gaugeFill_ = std::make_unique<Sprite>(engine);
    damageFlash_ = std::make_unique<Sprite>(engine);
}

void PlayerHealth::BindProperties(PropertyBinder& binder)
{
    binder.Bind("Health_Max", &maxHP_, 100, 1.0f, 1, 10000);

    binder.Bind("Health_GaugeOffset", &gaugeOffset_, { 40.0f, -40.0f }, 1.0f);
    binder.Bind("Health_GaugeSize", &gaugeSize_, { 300.0f, 10.0f }, 1.0f, 1.0f, 1280.0f);
    binder.BindColor("Health_GaugeColor", &gaugeColor_, { 0.9f, 0.9f, 0.9f, 0.9f });
    binder.BindColor("Health_LowColor", &lowHealthColor_, { 0.9f, 0.2f, 0.15f, 0.9f });
    binder.Bind("Health_LowRatio", &lowHealthRatio_, 0.3f, 0.01f, 0.0f, 1.0f);

    binder.BindColor("Health_FlashColor", &flashColor_, { 0.8f, 0.0f, 0.0f, 0.35f });
    binder.Bind("Health_FlashDuration", &flashDuration_, 0.35f, 0.01f, 0.01f, 3.0f);

    Refill();
}

void PlayerHealth::DrawProperties(PropertyBinder& binder)
{
#ifdef ENABLE_IMGUI
    ImGui::Text("現在: %d / %d", currentHP_, maxHP_);
    if (ImGui::Button("全回復"))
    {
        Refill();
    }
    binder.Draw("Health_Max", "最大 HP");
    binder.Draw("Health_GaugeOffset", "ゲージの位置 (画面左下から)");
    binder.Draw("Health_GaugeSize", "ゲージの大きさ");
    binder.Draw("Health_GaugeColor", "ゲージの色");
    binder.Draw("Health_LowColor", "残り少ないときの色");
    binder.Draw("Health_LowRatio", "残り少ないとみなす割合");
    binder.Draw("Health_FlashColor", "被弾時の画面の色");
    binder.Draw("Health_FlashDuration", "被弾時の色が消えるまで (秒)");
#endif
}

void PlayerHealth::Update()
{
    flashTimer_ = std::max(flashTimer_ - TimeManager::GetInstance()->GetDeltaTime(), 0.0f);
}

void PlayerHealth::TakeDamage(int damage)
{
    currentHP_ = std::max(currentHP_ - damage, 0);
    flashTimer_ = flashDuration_;
}

void PlayerHealth::Refill()
{
    currentHP_ = maxHP_;
    flashTimer_ = 0.0f;
}

void PlayerHealth::DrawHUD()
{
    if (flashTimer_ > 0.0f)
    {
        Vector4 color = flashColor_;
        color.w *= flashTimer_ / flashDuration_;

        damageFlash_->SetScreenAnchor({ 0.0f, 0.0f });
        damageFlash_->SetAnchorPoint({ 0.0f, 0.0f });
        damageFlash_->SetPosition({ 0.0f, 0.0f });
        damageFlash_->SetSize({ Engine::GetUIWidth(), Engine::GetUIHeight() });
        damageFlash_->SetColor(color);
        damageFlash_->SetLayerOrder(9);
        damageFlash_->Draw();
    }

    const float healthRatio = static_cast<float>(currentHP_) / static_cast<float>(maxHP_);

    gaugeBack_->SetScreenAnchor({ 0.0f, 1.0f });
    gaugeBack_->SetAnchorPoint({ 0.0f, 0.5f });
    gaugeBack_->SetPosition(gaugeOffset_);
    gaugeBack_->SetSize(gaugeSize_);
    gaugeBack_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.5f));
    gaugeBack_->SetLayerOrder(10);
    gaugeBack_->Draw();

    gaugeFill_->SetScreenAnchor({ 0.0f, 1.0f });
    gaugeFill_->SetAnchorPoint({ 0.0f, 0.5f });
    gaugeFill_->SetPosition(gaugeOffset_);
    gaugeFill_->SetSize({ gaugeSize_.x * healthRatio, gaugeSize_.y });
    gaugeFill_->SetColor(healthRatio < lowHealthRatio_ ? lowHealthColor_ : gaugeColor_);
    gaugeFill_->SetLayerOrder(11);
    gaugeFill_->Draw();
}
