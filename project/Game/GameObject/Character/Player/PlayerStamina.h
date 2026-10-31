#pragma once
#include "PropertyBinder.h"
#include "Sprite.h"

// スタミナ。ダッシュ中は減り続け、回避で一度に減る。使うのをやめて少したつと回復する
class PlayerStamina
{
public:
    PlayerStamina(FS::Engine* engine);

    void BindProperties(FS::PropertyBinder& binder);
    void DrawProperties(FS::PropertyBinder& binder);

    void Update();
    void DrawGauge();

    // 足りていれば amount だけ減らして true を返す
    bool TryConsume(float amount);
    // 1 秒あたり amountPerSecond の割合で減らす。0 になると息切れする
    void Drain(float amountPerSecond);

    // 息切れ中はダッシュできない
    bool IsExhausted() const { return isExhausted_; }
    float GetCurrent() const { return currentStamina_; }

private:
    float maxStamina_ = 100.0f;
    float regenPerSecond_ = 30.0f;
    float regenDelay_ = 0.8f;              // 使ってから回復が始まるまでの時間
    float exhaustedRecoverAmount_ = 40.0f; // 息切れしたあと、ここまで回復するとまたダッシュできる

    float currentStamina_ = 100.0f;
    float timeSinceUsed_ = 0.0f;
    bool isExhausted_ = false;

    // 画面下のゲージ
    std::unique_ptr<FS::Sprite> gaugeBack_;
    std::unique_ptr<FS::Sprite> gaugeFill_;
    FS::Vector2 gaugeOffset_ = { 0.0f, -60.0f };   // 画面下中央からのずれ
    FS::Vector2 gaugeSize_ = { 240.0f, 6.0f };
    FS::Vector4 gaugeColor_ = { 1.0f, 1.0f, 1.0f, 0.9f };
    FS::Vector4 exhaustedColor_ = { 0.9f, 0.25f, 0.2f, 0.9f };
};
