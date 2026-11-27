#include "pch.h"
#include "PlayerAmmoDisplay.h"
#include "UITextStyle.h"
#include "PlayerWeapon.h"
#include "TimeManager.h"
#include "GameInput.h"

using namespace FS;

PlayerAmmoDisplay::PlayerAmmoDisplay(Engine* engine)
    : engine_(engine)
{
    // 数字と文字は画面の左下を基準にし、右端をそろえる
    ammoText_ = std::make_unique<Text>(engine_, "Title");
    UITextStyle::ApplyNumberShadow(*ammoText_);
    ammoText_->SetScreenAnchor({ 0.0f, 1.0f });
    ammoText_->SetPivot({ 1.0f, 1.0f });
    ammoText_->SetAlign(TextAlign::Right);
    ammoText_->SetLayerOrder(12);

    magazineText_ = std::make_unique<Text>(engine_, "Title");
    UITextStyle::ApplyNumberShadow(*magazineText_);
    magazineText_->SetScreenAnchor({ 0.0f, 1.0f });
    magazineText_->SetPivot({ 1.0f, 1.0f });
    magazineText_->SetAlign(TextAlign::Right);
    magazineText_->SetLayerOrder(12);

    promptText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*promptText_);
    promptText_->SetAnchor(UIAnchor::Center);
    promptText_->SetAlign(TextAlign::Center);
    promptText_->SetLayerOrder(12);

    ammoTypeText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*ammoTypeText_);
    ammoTypeText_->SetScreenAnchor({ 0.0f, 1.0f });
    ammoTypeText_->SetPivot({ 1.0f, 1.0f });
    ammoTypeText_->SetAlign(TextAlign::Right);
    ammoTypeText_->SetLayerOrder(12);

    reloadText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*reloadText_);
    reloadText_->SetAnchor(UIAnchor::Center);
    reloadText_->SetAlign(TextAlign::Center);
    reloadText_->SetLayerOrder(12);

    roundSprite_ = std::make_unique<Sprite>(engine_);
    roundSprite_->SetScreenAnchor({ 0.0f, 1.0f });
    roundSprite_->SetAnchorPoint({ 0.0f, 1.0f });
    roundSprite_->SetLayerOrder(12);

    promptIcon_ = std::make_unique<Sprite>(engine_);
    promptIcon_->SetScreenAnchor({ 0.5f, 0.5f });
    promptIcon_->SetAnchorPoint({ 0.5f, 0.5f });
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
    binder.Bind("Ammo_TextOffset", &ammoTextOffset_, { 75.0f, -45.0f }, 1.0f);
    binder.Bind("Ammo_FontSize", &ammoFontSize_, 56.0f, 0.5f, 4.0f, 200.0f);
    binder.Bind("Ammo_MagazineTextOffset", &magazineTextOffset_, { 125.0f, -55.0f }, 1.0f);
    binder.Bind("Ammo_MagazineFontSize", &magazineFontSize_, 26.0f, 0.5f, 4.0f, 200.0f);
    binder.Bind("Ammo_RoundsOffset", &roundsOffset_, { 35.5f, -111.0f }, 1.0f);
    binder.Bind("Ammo_RoundSize", &roundSize_, { 6.0f, 18.0f }, 0.5f, 1.0f, 100.0f);
    binder.Bind("Ammo_RoundSpacing", &roundSpacing_, 4.0f, 0.5f, 0.0f, 50.0f);
    binder.Bind("Ammo_PromptCenterOffset", &promptOffset_, { 0.0f, 90.0f }, 1.0f);
    binder.Bind("Ammo_PromptFontSize", &promptFontSize_, 24.0f, 0.5f, 4.0f, 200.0f);
    binder.Bind("Ammo_PromptIconSize", &promptIconSize_, 36.0f, 0.5f, 4.0f, 200.0f);
    binder.Bind("Ammo_PromptIconGap", &promptIconGap_, 8.0f, 0.5f, -100.0f, 200.0f);
    binder.Bind("Ammo_ReloadBarOffset", &reloadBarOffset_, { 0.0f, 90.0f }, 1.0f);
    binder.Bind("Ammo_ReloadBarSize", &reloadBarSize_, { 70.0f, 3.0f }, 0.5f, 1.0f, 1000.0f);
    binder.Bind("Ammo_ReloadTextOffset", &reloadTextOffset_, { 0.0f, 100.0f }, 1.0f);
    binder.Bind("Ammo_ReloadFontSize", &reloadFontSize_, 16.0f, 0.5f, 4.0f, 200.0f);
    binder.BindColor("Ammo_NormalColor", &normalColor_, { 1.0f, 1.0f, 1.0f, 0.95f });
    binder.BindColor("Ammo_LowColor", &lowAmmoColor_, { 1.0f, 0.35f, 0.25f, 0.95f });
    binder.BindColor("Ammo_EmptyRoundColor", &emptyRoundColor_, { 1.0f, 1.0f, 1.0f, 0.2f });
    binder.Bind("Ammo_TypeTextOffset", &ammoTypeTextOffset_, { 208.0f, -59.0f }, 1.0f);
    binder.Bind("Ammo_TypeFontSize", &ammoTypeFontSize_, 22.0f, 0.5f, 4.0f, 200.0f);
    binder.BindColor("Ammo_IncendiaryColor", &incendiaryColor_, { 1.0f, 0.55f, 0.15f, 0.95f });
    binder.Bind("Ammo_LowRatio", &lowAmmoRatio_, 0.3f, 0.01f, 0.0f, 1.0f);

    binder.BindFontName("Ammo_NumberFont", ammoText_->GetFontName(), [this](const std::string& fontName)
        {
            ammoText_->SetFont(fontName);
            magazineText_->SetFont(fontName);
        });
    binder.BindFontName("Ammo_LabelFont", ammoTypeText_->GetFontName(), [this](const std::string& fontName)
        {
            ammoTypeText_->SetFont(fontName);
            promptText_->SetFont(fontName);
            reloadText_->SetFont(fontName);
        });
}

void PlayerAmmoDisplay::DrawProperties(PropertyBinder& binder)
{
    binder.Draw("Ammo_TextOffset", "残弾の数字の位置 (画面左下から、右端)");
    binder.Draw("Ammo_FontSize", "残弾の数字の大きさ");
    binder.Draw("Ammo_MagazineTextOffset", "装弾数の位置 (画面左下から、右端)");
    binder.Draw("Ammo_MagazineFontSize", "装弾数の大きさ");
    binder.Draw("Ammo_RoundsOffset", "弾の列の位置 (画面左下から、左端)");
    binder.Draw("Ammo_RoundSize", "弾 1 発の大きさ");
    binder.Draw("Ammo_RoundSpacing", "弾の間隔");
    binder.Draw("Ammo_PromptCenterOffset", "弾切れの表示の位置 (画面中央から)");
    binder.Draw("Ammo_PromptFontSize", "弾切れの表示の文字の大きさ");
    binder.Draw("Ammo_PromptIconSize", "ボタン画像の大きさ");
    binder.Draw("Ammo_PromptIconGap", "ボタン画像と文字の間");
    binder.Draw("Ammo_ReloadBarOffset", "リロードのゲージの位置 (画面中央から)");
    binder.Draw("Ammo_ReloadBarSize", "リロードのゲージの大きさ");
    binder.Draw("Ammo_ReloadTextOffset", "「リロード中」の位置 (画面中央から)");
    binder.Draw("Ammo_ReloadFontSize", "「リロード中」の文字の大きさ");
    binder.Draw("Ammo_NormalColor", "色");
    binder.Draw("Ammo_LowColor", "残り少ないときの色");
    binder.Draw("Ammo_EmptyRoundColor", "撃ったあとの弾の色");
    binder.Draw("Ammo_TypeTextOffset", "弾の種類の位置 (画面左下から、右端)");
    binder.Draw("Ammo_TypeFontSize", "弾の種類の文字の大きさ");
    binder.Draw("Ammo_IncendiaryColor", "焼夷弾の色");
    binder.Draw("Ammo_LowRatio", "残り少ないとみなす割合");
    binder.Draw("Ammo_NumberFont", "数字 (残弾と装弾数) のフォント");
    binder.Draw("Ammo_LabelFont", "文字 (弾の種類とリロードの表示) のフォント。Title は日本語が出ない");
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

    // 焼夷弾のときは、種類の文字と弾の列を炎の色にする
    const bool isIncendiary = weapon.GetAmmoType() == PlayerWeapon::AmmoType::Incendiary;
    ammoTypeText_->SetText(isIncendiary ? "焼夷弾" : "通常弾");
    ammoTypeText_->SetPosition(ammoTypeTextOffset_);
    ammoTypeText_->SetFontSize(ammoTypeFontSize_);
    ammoTypeText_->SetColor(isIncendiary ? incendiaryColor_ : normalColor_);
    ammoTypeText_->Draw();

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

    // 左端をそろえて右へ並べる
    for (int roundIndex = 0; roundIndex < magazineSize; ++roundIndex)
    {
        const float offsetX = static_cast<float>(roundIndex) * (roundSize_.x + roundSpacing_);
        const bool isFilled = roundIndex < filledCount;

        roundSprite_->SetPosition({ roundsOffset_.x + offsetX, roundsOffset_.y });
        const Vector4& filledColor = (weapon.GetAmmoType() == PlayerWeapon::AmmoType::Incendiary) ? incendiaryColor_ : normalColor_;
        roundSprite_->SetColor(isFilled ? filledColor : emptyRoundColor_);
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

    // 弾切れ
    const float blinkOpacity = 0.6f + 0.4f * (std::cos(elapsedTime_ * 6.0f) * 0.5f + 0.5f);

    promptText_->SetText("リロード");
    promptText_->SetFontSize(promptFontSize_);
    promptText_->SetColor(lowAmmoColor_);
    promptText_->SetOpacity(blinkOpacity);

    // ボタンの絵と文字をひとまとまりにして、真ん中を位置に合わせる
    const float textWidth = promptText_->MeasureSize().x;
    const float groupLeft = promptOffset_.x - (promptIconSize_ + promptIconGap_ + textWidth) * 0.5f;
    promptText_->SetPosition({ groupLeft + promptIconSize_ + promptIconGap_ + textWidth * 0.5f, promptOffset_.y });
    promptText_->Draw();

    const bool isUsingController = GameInput::IsUsingController();
    promptIcon_->SetTexture(isUsingController ? "xbox_button_x" : "keyboard_r");
    promptIcon_->SetPosition({ groupLeft + promptIconSize_ * 0.5f, promptOffset_.y });
    promptIcon_->SetSize({ promptIconSize_, promptIconSize_ });
    promptIcon_->Draw();
}
