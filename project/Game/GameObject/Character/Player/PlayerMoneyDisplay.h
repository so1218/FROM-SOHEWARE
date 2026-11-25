#pragma once
#include "PropertyBinder.h"
#include "Text.h"
#include "Sprite.h"

// 所持金
class PlayerMoneyDisplay
{
public:
    PlayerMoneyDisplay(FS::Engine* engine);

    void BindProperties(FS::PropertyBinder& binder);
    void DrawProperties(FS::PropertyBinder& binder);

    // 拾ったときに画像を弾ませる
    void OnGain(int amount);
    // 弾ませている途中なら止める
    void Reset();
    void Draw(int money);

private:
    std::unique_ptr<FS::Text> moneyText_;
    std::unique_ptr<FS::Sprite> iconSprite_;

    float popTimer_ = 1000.0f;

    // パラメータ
    FS::Vector2 moneyOffset_ = { -40.0f, 40.0f };
    float moneyFontSize_ = 40.0f;
    FS::Vector4 moneyColor_ = { 1.0f, 1.0f, 1.0f, 0.95f };

    // アイコン
    std::string iconTextureName_ = "pouch";
    uint32_t iconTextureHandle_ = 0;
    float iconScale_ = 1.0f;    
    float iconGap_ = 10.0f;    
    float iconOffsetY_ = 0.0f;  
    FS::Vector4 iconColor_ = { 1.0f, 1.0f, 1.0f, 0.95f };
    // 拾ったときに一番大きくなる倍率と、元に戻るまでの時間
    float iconPopScale_ = 1.6f;
    float iconPopTime_ = 0.35f;
};
