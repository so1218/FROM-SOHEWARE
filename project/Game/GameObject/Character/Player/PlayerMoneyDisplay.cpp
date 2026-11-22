#include "pch.h"
#include "PlayerMoneyDisplay.h"
#include "TimeManager.h"
#include "ImGuiManager.h"

using namespace FS;

PlayerMoneyDisplay::PlayerMoneyDisplay(Engine* engine)
{
    // 数字の左に置くので、右端の縦の真ん中を基準にする
    iconSprite_ = std::make_unique<Sprite>(engine);
    iconSprite_->SetScreenAnchor({ 1.0f, 0.0f });
    iconSprite_->SetAnchorPoint({ 1.0f, 0.5f });
    iconSprite_->SetLayerOrder(12);

    moneyText_ = std::make_unique<Text>(engine);
    moneyText_->SetAnchor(UIAnchor::TopRight);
    moneyText_->SetAlign(TextAlign::Right);
    moneyText_->SetLayerOrder(12);

    gainText_ = std::make_unique<Text>(engine);
    gainText_->SetAnchor(UIAnchor::TopRight);
    gainText_->SetAlign(TextAlign::Right);
    gainText_->SetLayerOrder(12);
}

void PlayerMoneyDisplay::BindProperties(PropertyBinder& binder)
{
    binder.Bind("Money_Offset", &moneyOffset_, moneyOffset_, 1.0f);
    binder.Bind("Money_FontSize", &moneyFontSize_, moneyFontSize_, 0.5f, 4.0f, 200.0f);
    binder.Bind("Money_GainOffset", &gainOffset_, gainOffset_, 1.0f);
    binder.Bind("Money_GainFontSize", &gainFontSize_, gainFontSize_, 0.5f, 4.0f, 200.0f);
    binder.Bind("Money_GainShowTime", &gainShowTime_, gainShowTime_, 0.05f, 0.1f, 10.0f);
    binder.Bind("Money_GainRise", &gainRiseDistance_, gainRiseDistance_, 0.5f, 0.0f, 200.0f);
    binder.BindColor("Money_Color", &moneyColor_, moneyColor_);
    binder.BindColor("Money_GainColor", &gainColor_, gainColor_);

    binder.BindTexture("Money_IconTexture", &iconTextureName_, &iconTextureHandle_, "pouch", TextureType::UI);
    binder.Bind("Money_IconScale", &iconScale_, iconScale_, 0.01f, 0.1f, 5.0f);
    binder.Bind("Money_IconGap", &iconGap_, iconGap_, 0.5f, -100.0f, 200.0f);
    binder.Bind("Money_IconOffsetY", &iconOffsetY_, iconOffsetY_, 0.5f, -100.0f, 100.0f);
    binder.BindColor("Money_IconColor", &iconColor_, iconColor_);
}

void PlayerMoneyDisplay::DrawProperties(PropertyBinder& binder)
{
    binder.Draw("Money_Offset", "所持金の位置 (右上から)");
    binder.Draw("Money_FontSize", "所持金の文字の大きさ");
    binder.Draw("Money_GainOffset", "「+」の位置 (右上から)");
    binder.Draw("Money_GainFontSize", "「+」の文字の大きさ");
    binder.Draw("Money_GainShowTime", "「+」を出す時間 (秒)");
    binder.Draw("Money_GainRise", "「+」が上に動く量");
    binder.Draw("Money_Color", "所持金の色");
    binder.Draw("Money_GainColor", "「+」の色");

    ImGui::SeparatorText("アイコン");
    binder.Draw("Money_IconTexture", "画像");
    binder.Draw("Money_IconScale", "大きさ (数字の文字の大きさに対して)");
    binder.Draw("Money_IconGap", "数字との間");
    binder.Draw("Money_IconOffsetY", "上下のずれ");
    binder.Draw("Money_IconColor", "色");
}

void PlayerMoneyDisplay::OnGain(int amount)
{
    if (gainTimer_ <= 0.0f)
    {
        gainAmount_ = 0;
    }
    gainAmount_ += amount;
    gainTimer_ = gainShowTime_;
}

void PlayerMoneyDisplay::Draw(int money)
{
    moneyText_->SetText(std::to_string(money));
    moneyText_->SetPosition(moneyOffset_);
    moneyText_->SetFontSize(moneyFontSize_);
    moneyText_->SetColor(moneyColor_);

    // アイコンは数字の左に、数字の高さの真ん中にそろえて置く
    const Vector2 textSize = moneyText_->MeasureSize();
    const float iconSize = moneyFontSize_ * iconScale_;

    iconSprite_->SetTextureHandle(iconTextureHandle_);
    iconSprite_->SetSize({ iconSize, iconSize });
    iconSprite_->SetPosition({ moneyOffset_.x - textSize.x - iconGap_, moneyOffset_.y + textSize.y * 0.5f + iconOffsetY_ });
    iconSprite_->SetColor(iconColor_);
    iconSprite_->Draw();

    moneyText_->Draw();

    if (gainTimer_ <= 0.0f)
    {
        return;
    }
    gainTimer_ -= TimeManager::GetInstance()->GetDeltaTime();

    // 出てすぐははっきり見せ、最後の 3 割で消していく
    const float remainingRatio = std::clamp(gainTimer_ / gainShowTime_, 0.0f, 1.0f);
    Vector4 color = gainColor_;
    color.w *= std::min(remainingRatio / 0.3f, 1.0f);

    gainText_->SetText("+" + std::to_string(gainAmount_));
    gainText_->SetPosition({ gainOffset_.x, gainOffset_.y - gainRiseDistance_ * (1.0f - remainingRatio) });
    gainText_->SetFontSize(gainFontSize_);
    gainText_->SetColor(color);
    gainText_->Draw();
}
