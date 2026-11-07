#pragma once
#include "PropertyBinder.h"
#include "Text.h"

// 画面右上の所持金。拾ったときは下に「+30」を少しのあいだ出す
class PlayerMoneyDisplay
{
public:
    PlayerMoneyDisplay(FS::Engine* engine);

    void BindProperties(FS::PropertyBinder& binder);
    void DrawProperties(FS::PropertyBinder& binder);

    // 続けて拾ったときは、表示中の「+」に足していく
    void OnGain(int amount);
    void Draw(int money);

private:
    std::unique_ptr<FS::Text> moneyText_;
    std::unique_ptr<FS::Text> gainText_;

    int gainAmount_ = 0;
    float gainTimer_ = 0.0f;   // 0 より大きい間「+」を出す

    // パラメータ (位置は画面右上から)
    FS::Vector2 moneyOffset_ = { -40.0f, 40.0f };
    float moneyFontSize_ = 40.0f;
    FS::Vector2 gainOffset_ = { -40.0f, 90.0f };
    float gainFontSize_ = 28.0f;
    float gainShowTime_ = 1.2f;
    float gainRiseDistance_ = 12.0f;   // 消えるまでに上に動く量
    FS::Vector4 moneyColor_ = { 1.0f, 1.0f, 1.0f, 0.95f };
    FS::Vector4 gainColor_ = { 1.0f, 0.85f, 0.35f, 1.0f };
};
