#include "pch.h"
#include "PlayerMoneyDisplay.h"
#include "TimeManager.h"

using namespace FS;

PlayerMoneyDisplay::PlayerMoneyDisplay(Engine* engine)
{
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
