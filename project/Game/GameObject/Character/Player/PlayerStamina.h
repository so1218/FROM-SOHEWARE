#pragma once
#include "PropertyBinder.h"
#include "Sprite.h"

class PlayerStamina
{
public:
    PlayerStamina(FS::Engine* engine);

    void BindProperties(FS::PropertyBinder& binder);
    void DrawProperties(FS::PropertyBinder& binder);

    void Update();
    // HP のゲージの真下に、左端をそろえて出す
    void DrawGauge(const FS::Vector2& healthGaugeOffset, const FS::Vector2& healthGaugeSize);

    // 足りていれば amount だけ減らして true を返す
    bool TryConsume(float amount);
    void Refill();
    // 1 秒あたり amountPerSecond の割合で減らす
    void Drain(float amountPerSecond);

    // 息切れ中はダッシュできない
    bool IsExhausted() const { return isExhausted_; }
    float GetCurrent() const { return currentStamina_; }

private:
    float maxStamina_ = 100.0f;
    float regenPerSecond_ = 30.0f;
    float regenDelay_ = 0.8f;              
    float exhaustedRecoverAmount_ = 40.0f; 

    float currentStamina_ = 100.0f;
    float timeSinceUsed_ = 0.0f;
    bool isExhausted_ = false;

    // HP の下のゲージ
    std::unique_ptr<FS::Sprite> gaugeBack_;
    std::unique_ptr<FS::Sprite> gaugeFill_;
    float gaugeGap_ = 6.0f;
    bool isHiddenWhenFull_ = false;
    FS::Vector2 gaugeSize_ = { 240.0f, 6.0f };
    FS::Vector4 gaugeColor_ = { 0.95f, 0.78f, 0.25f, 0.95f };
    FS::Vector4 exhaustedColor_ = { 0.9f, 0.25f, 0.2f, 0.9f };
};
