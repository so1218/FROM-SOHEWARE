#pragma once
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

class PlayerWeapon;

// 画面右下の残弾表示。数字と弾の列を出し、弾切れのときはリロードの操作を出す
// リロードの進み具合は、プレイヤーが見ている照準の下に出す
class PlayerAmmoDisplay
{
public:
    PlayerAmmoDisplay(FS::Engine* engine);

    void BindProperties(FS::PropertyBinder& binder);
    void DrawProperties(FS::PropertyBinder& binder);

    void Draw(const PlayerWeapon& weapon);

private:
    void DrawRounds(const PlayerWeapon& weapon);
    void DrawReloadPrompt(const PlayerWeapon& weapon);
    void DrawReloadProgress(const PlayerWeapon& weapon);

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::Text> ammoText_;
    std::unique_ptr<FS::Text> magazineText_;
    std::unique_ptr<FS::Text> promptText_;
    std::unique_ptr<FS::Text> reloadText_;
    std::unique_ptr<FS::Sprite> roundSprite_;
    std::unique_ptr<FS::Sprite> promptIcon_;
    std::unique_ptr<FS::Sprite> reloadBarBack_;
    std::unique_ptr<FS::Sprite> reloadBarFill_;
    float elapsedTime_ = 0.0f;

    // パラメータ (位置はすべて画面右下から)
    FS::Vector2 ammoTextOffset_ = { -110.0f, -36.0f };
    float ammoFontSize_ = 56.0f;
    FS::Vector2 magazineTextOffset_ = { -40.0f, -44.0f };
    float magazineFontSize_ = 26.0f;

    FS::Vector2 roundsOffset_ = { -40.0f, -110.0f };   // 弾の列の右端
    FS::Vector2 roundSize_ = { 6.0f, 18.0f };
    float roundSpacing_ = 4.0f;

    FS::Vector2 promptOffset_ = { -40.0f, -150.0f };
    float promptFontSize_ = 24.0f;
    float promptIconSize_ = 36.0f;

    // リロードの進み具合。位置は画面の中央から。照準が最大まで広がっても重ならない所に置く
    FS::Vector2 reloadBarOffset_ = { 0.0f, 90.0f };
    FS::Vector2 reloadBarSize_ = { 70.0f, 3.0f };
    FS::Vector2 reloadTextOffset_ = { 0.0f, 100.0f };
    float reloadFontSize_ = 16.0f;

    FS::Vector4 normalColor_ = { 1.0f, 1.0f, 1.0f, 0.95f };
    FS::Vector4 lowAmmoColor_ = { 1.0f, 0.35f, 0.25f, 0.95f };
    FS::Vector4 emptyRoundColor_ = { 1.0f, 1.0f, 1.0f, 0.2f };
    float lowAmmoRatio_ = 0.3f;   // この割合以下になると赤くする
};
