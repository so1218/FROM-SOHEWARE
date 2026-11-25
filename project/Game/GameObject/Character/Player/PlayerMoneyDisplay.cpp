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
}

void PlayerMoneyDisplay::BindProperties(PropertyBinder& binder)
{
    binder.Bind("Money_Offset", &moneyOffset_, moneyOffset_, 1.0f);
    binder.Bind("Money_FontSize", &moneyFontSize_, moneyFontSize_, 0.5f, 4.0f, 200.0f);
    binder.BindColor("Money_Color", &moneyColor_, moneyColor_);
    binder.BindFontName("Money_Font", moneyText_->GetFontName(), [this](const std::string& fontName)
        {
            moneyText_->SetFont(fontName);
        });

    binder.BindTexture("Money_IconTexture", &iconTextureName_, &iconTextureHandle_, "pouch", TextureType::UI);
    binder.Bind("Money_IconScale", &iconScale_, iconScale_, 0.01f, 0.1f, 5.0f);
    binder.Bind("Money_IconGap", &iconGap_, iconGap_, 0.5f, -100.0f, 200.0f);
    binder.Bind("Money_IconOffsetY", &iconOffsetY_, iconOffsetY_, 0.5f, -100.0f, 100.0f);
    binder.BindColor("Money_IconColor", &iconColor_, iconColor_);
    binder.Bind("Money_IconPopScale", &iconPopScale_, iconPopScale_, 0.01f, 1.0f, 5.0f);
    binder.Bind("Money_IconPopTime", &iconPopTime_, iconPopTime_, 0.01f, 0.05f, 3.0f);
}

void PlayerMoneyDisplay::DrawProperties(PropertyBinder& binder)
{
    binder.Draw("Money_Offset", "所持金の位置 (右上から)");
    binder.Draw("Money_FontSize", "所持金の文字の大きさ");
    binder.Draw("Money_Color", "所持金の色");
    binder.Draw("Money_Font", "所持金のフォント");

    ImGui::SeparatorText("アイコン");
    binder.Draw("Money_IconTexture", "画像");
    binder.Draw("Money_IconScale", "大きさ (数字の文字の大きさに対して)");
    binder.Draw("Money_IconGap", "数字との間");
    binder.Draw("Money_IconOffsetY", "上下のずれ");
    binder.Draw("Money_IconColor", "色");
    binder.Draw("Money_IconPopScale", "拾ったときの大きさ (倍)");
    binder.Draw("Money_IconPopTime", "拾ってから元の大きさに戻るまで (秒)");
}

void PlayerMoneyDisplay::Reset()
{
    popTimer_ = iconPopTime_;
}

void PlayerMoneyDisplay::OnGain(int)
{
    // 続けて拾ったときは、その度に弾ませ直す
    popTimer_ = 0.0f;
}

void PlayerMoneyDisplay::Draw(int money)
{
    moneyText_->SetText(std::to_string(money));
    moneyText_->SetPosition(moneyOffset_);
    moneyText_->SetFontSize(moneyFontSize_);
    moneyText_->SetColor(moneyColor_);

    // アイコンは数字の左に、数字の高さの真ん中にそろえて置く
    const Vector2 textSize = moneyText_->MeasureSize();
    // 拾った瞬間にすばやく膨らみ、ゆっくり元の大きさに戻る
    popTimer_ += TimeManager::GetInstance()->GetDeltaTime();
    const float popRatio = std::clamp(popTimer_ / std::max(iconPopTime_, 0.001f), 0.0f, 1.0f);
    constexpr float kGrowRatio = 0.2f;
    const float growRatio = std::min(popRatio / kGrowRatio, 1.0f);
    const float shrinkRatio = std::max((popRatio - kGrowRatio) / (1.0f - kGrowRatio), 0.0f);
    const float shrinkInverse = 1.0f - shrinkRatio;
    const float pop = growRatio * shrinkInverse * shrinkInverse;
    const float baseIconSize = moneyFontSize_ * iconScale_;
    const float iconSize = baseIconSize * (1.0f + (iconPopScale_ - 1.0f) * pop);
    // 右端を基準に置いているので、画像の中心から膨らむようにずらす
    const float popShiftX = (iconSize - baseIconSize) * 0.5f;

    iconSprite_->SetTextureHandle(iconTextureHandle_);
    iconSprite_->SetSize({ iconSize, iconSize });
    iconSprite_->SetPosition({ moneyOffset_.x - textSize.x - iconGap_ + popShiftX, moneyOffset_.y + textSize.y * 0.5f + iconOffsetY_ });
    iconSprite_->SetColor(iconColor_);
    iconSprite_->Draw();

    moneyText_->Draw();
}
