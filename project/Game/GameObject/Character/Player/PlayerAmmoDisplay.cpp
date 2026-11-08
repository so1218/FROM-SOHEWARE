#include "pch.h"
#include "PlayerAmmoDisplay.h"
#include "PlayerWeapon.h"
#include "TimeManager.h"
#include "GameInput.h"

using namespace FS;

PlayerAmmoDisplay::PlayerAmmoDisplay(Engine* engine)
    : engine_(engine)
{
    ammoText_ = std::make_unique<Text>(engine_);
    ammoText_->SetAnchor(UIAnchor::BottomRight);
    ammoText_->SetAlign(TextAlign::Right);
    ammoText_->SetLayerOrder(12);

    magazineText_ = std::make_unique<Text>(engine_);
    magazineText_->SetAnchor(UIAnchor::BottomRight);
    magazineText_->SetAlign(TextAlign::Right);
    magazineText_->SetLayerOrder(12);

    promptText_ = std::make_unique<Text>(engine_);
    promptText_->SetAnchor(UIAnchor::BottomRight);
    promptText_->SetAlign(TextAlign::Right);
    promptText_->SetLayerOrder(12);

    reloadText_ = std::make_unique<Text>(engine_);
    reloadText_->SetAnchor(UIAnchor::Center);
    reloadText_->SetAlign(TextAlign::Center);
    reloadText_->SetLayerOrder(12);

    roundSprite_ = std::make_unique<Sprite>(engine_);
    roundSprite_->SetScreenAnchor({ 1.0f, 1.0f });
    roundSprite_->SetAnchorPoint({ 1.0f, 1.0f });
    roundSprite_->SetLayerOrder(12);

    promptIcon_ = std::make_unique<Sprite>(engine_);
    promptIcon_->SetScreenAnchor({ 1.0f, 1.0f });
    promptIcon_->SetAnchorPoint({ 1.0f, 1.0f });
    promptIcon_->SetLayerOrder(12);

    // ゲージは左端から伸ばすので、左端を基準にする
    reloadBarBack_ = std::make_unique<Sprite>(engine_);
    reloadBarBack_->SetScreenAnchor({ 0.5f, 0.5f });
    reloadBarBack_->SetAnchorPoint({ 0.0f, 0.5f });
    reloadBarBack_->SetLayerOrder(12);

    reloadBarFill_ = std::make_unique<Sprite>(engine_);
    reloadBarFill_->SetScreenAnchor({ 0.5f, 0.5f });
    reloadBarFill_->SetAnchorPoint({ 0.0f, 0.5f });
    reloadBarFill_->SetLayerOrder(13);
}

void PlayerAmmoDisplay::BindProperties(PropertyBinder& binder)
{
    binder.Bind("Ammo_TextOffset", &ammoTextOffset_, { -110.0f, -36.0f }, 1.0f);
    binder.Bind("Ammo_FontSize", &ammoFontSize_, 56.0f, 0.5f, 4.0f, 200.0f);
    binder.Bind("Ammo_MagazineTextOffset", &magazineTextOffset_, { -40.0f, -44.0f }, 1.0f);
    binder.Bind("Ammo_MagazineFontSize", &magazineFontSize_, 26.0f, 0.5f, 4.0f, 200.0f);
    binder.Bind("Ammo_RoundsOffset", &roundsOffset_, { -40.0f, -110.0f }, 1.0f);
    binder.Bind("Ammo_RoundSize", &roundSize_, { 6.0f, 18.0f }, 0.5f, 1.0f, 100.0f);
    binder.Bind("Ammo_RoundSpacing", &roundSpacing_, 4.0f, 0.5f, 0.0f, 50.0f);
    binder.Bind("Ammo_PromptOffset", &promptOffset_, { -40.0f, -150.0f }, 1.0f);
    binder.Bind("Ammo_PromptFontSize", &promptFontSize_, 24.0f, 0.5f, 4.0f, 200.0f);
    binder.Bind("Ammo_PromptIconSize", &promptIconSize_, 36.0f, 0.5f, 4.0f, 200.0f);
    binder.Bind("Ammo_ReloadBarOffset", &reloadBarOffset_, { 0.0f, 90.0f }, 1.0f);
    binder.Bind("Ammo_ReloadBarSize", &reloadBarSize_, { 70.0f, 3.0f }, 0.5f, 1.0f, 1000.0f);
    binder.Bind("Ammo_ReloadTextOffset", &reloadTextOffset_, { 0.0f, 100.0f }, 1.0f);
    binder.Bind("Ammo_ReloadFontSize", &reloadFontSize_, 16.0f, 0.5f, 4.0f, 200.0f);
    binder.BindColor("Ammo_NormalColor", &normalColor_, { 1.0f, 1.0f, 1.0f, 0.95f });
    binder.BindColor("Ammo_LowColor", &lowAmmoColor_, { 1.0f, 0.35f, 0.25f, 0.95f });
    binder.BindColor("Ammo_EmptyRoundColor", &emptyRoundColor_, { 1.0f, 1.0f, 1.0f, 0.2f });
    binder.Bind("Ammo_LowRatio", &lowAmmoRatio_, 0.3f, 0.01f, 0.0f, 1.0f);
}

void PlayerAmmoDisplay::DrawProperties(PropertyBinder& binder)
{
    binder.Draw("Ammo_TextOffset", "残弾の数字の位置");
    binder.Draw("Ammo_FontSize", "残弾の数字の大きさ");
    binder.Draw("Ammo_MagazineTextOffset", "装弾数の位置");
    binder.Draw("Ammo_MagazineFontSize", "装弾数の大きさ");
    binder.Draw("Ammo_RoundsOffset", "弾の列の位置 (右端)");
    binder.Draw("Ammo_RoundSize", "弾 1 発の大きさ");
    binder.Draw("Ammo_RoundSpacing", "弾の間隔");
    binder.Draw("Ammo_PromptOffset", "弾切れの表示の位置 (右端)");
    binder.Draw("Ammo_PromptFontSize", "弾切れの表示の文字の大きさ");
    binder.Draw("Ammo_PromptIconSize", "ボタン画像の大きさ");
    binder.Draw("Ammo_ReloadBarOffset", "リロードのゲージの位置 (画面中央から)");
    binder.Draw("Ammo_ReloadBarSize", "リロードのゲージの大きさ");
    binder.Draw("Ammo_ReloadTextOffset", "「リロード中」の位置 (画面中央から)");
    binder.Draw("Ammo_ReloadFontSize", "「リロード中」の文字の大きさ");
    binder.Draw("Ammo_NormalColor", "色");
    binder.Draw("Ammo_LowColor", "残り少ないときの色");
    binder.Draw("Ammo_EmptyRoundColor", "撃ったあとの弾の色");
    binder.Draw("Ammo_LowRatio", "残り少ないとみなす割合");
}

void PlayerAmmoDisplay::Draw(const PlayerWeapon& weapon)
{
    elapsedTime_ += TimeManager::GetInstance()->GetDeltaTime();

    const int ammo = weapon.GetAmmo();
    const int magazineSize = weapon.GetMagazineSize();
    const bool isLow = ammo <= static_cast<int>(static_cast<float>(magazineSize) * lowAmmoRatio_);

    ammoText_->SetText(std::to_string(ammo));
    ammoText_->SetPosition(ammoTextOffset_);
    ammoText_->SetFontSize(ammoFontSize_);
    ammoText_->SetColor(isLow ? lowAmmoColor_ : normalColor_);
    ammoText_->Draw();

    magazineText_->SetText("/ " + std::to_string(magazineSize));
    magazineText_->SetPosition(magazineTextOffset_);
    magazineText_->SetFontSize(magazineFontSize_);
    magazineText_->SetColor(normalColor_);
    magazineText_->Draw();

    DrawRounds(weapon);
    DrawReloadPrompt(weapon);
    DrawReloadProgress(weapon);
}

void PlayerAmmoDisplay::DrawRounds(const PlayerWeapon& weapon)
{
    const int magazineSize = weapon.GetMagazineSize();

    // リロード中は弾が 1 発ずつ戻っていくように見せる
    int filledCount = weapon.GetAmmo();
    if (weapon.IsReloading())
    {
        filledCount = static_cast<int>(weapon.GetReloadProgress() * static_cast<float>(magazineSize));
    }

    roundSprite_->SetSize(roundSize_);

    // 右から並べる。右端が最後に撃つ弾
    for (int roundIndex = 0; roundIndex < magazineSize; ++roundIndex)
    {
        const float offsetX = -static_cast<float>(roundIndex) * (roundSize_.x + roundSpacing_);
        const bool isFilled = (magazineSize - 1 - roundIndex) < filledCount;

        roundSprite_->SetPosition({ roundsOffset_.x + offsetX, roundsOffset_.y });
        roundSprite_->SetColor(isFilled ? normalColor_ : emptyRoundColor_);
        roundSprite_->Draw();
    }
}

void PlayerAmmoDisplay::DrawReloadProgress(const PlayerWeapon& weapon)
{
    if (!weapon.IsReloading())
    {
        return;
    }

    const Vector2 barLeft = { reloadBarOffset_.x - reloadBarSize_.x * 0.5f, reloadBarOffset_.y };
    reloadBarBack_->SetPosition(barLeft);
    reloadBarBack_->SetSize(reloadBarSize_);
    reloadBarBack_->SetColor(emptyRoundColor_);
    reloadBarBack_->Draw();

    reloadBarFill_->SetPosition(barLeft);
    reloadBarFill_->SetSize({ reloadBarSize_.x * weapon.GetReloadProgress(), reloadBarSize_.y });
    reloadBarFill_->SetColor(normalColor_);
    reloadBarFill_->Draw();

    reloadText_->SetText("リロード中");
    reloadText_->SetPosition(reloadTextOffset_);
    reloadText_->SetFontSize(reloadFontSize_);
    reloadText_->SetColor(normalColor_);
    reloadText_->Draw();
}

void PlayerAmmoDisplay::DrawReloadPrompt(const PlayerWeapon& weapon)
{
    if (weapon.IsReloading() || weapon.GetAmmo() > 0)
    {
        return;
    }

    // 弾切れ。点滅させて目に入るようにする
    Vector4 color = lowAmmoColor_;
    color.w *= 0.6f + 0.4f * (std::cos(elapsedTime_ * 6.0f) * 0.5f + 0.5f);

    promptText_->SetText("リロード");
    promptText_->SetPosition(promptOffset_);
    promptText_->SetFontSize(promptFontSize_);
    promptText_->SetColor(color);
    promptText_->Draw();

    const bool isUsingController = GameInput::IsUsingController();
    const float textWidth = promptText_->MeasureSize().x;
    promptIcon_->SetTexture(isUsingController ? "xbox_button_x" : "keyboard_r");
    promptIcon_->SetPosition({ promptOffset_.x - textWidth - 8.0f, promptOffset_.y + (promptIconSize_ - promptFontSize_) * 0.5f });
    promptIcon_->SetSize({ promptIconSize_, promptIconSize_ });
    promptIcon_->Draw();
}
