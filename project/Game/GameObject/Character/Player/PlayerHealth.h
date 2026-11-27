#pragma once
#include "PropertyBinder.h"
#include "Sprite.h"

// プレイヤーの HP
class PlayerHealth
{
public:
    PlayerHealth(FS::Engine* engine);

    void BindProperties(FS::PropertyBinder& binder);
    void DrawProperties(FS::PropertyBinder& binder);

    void Update();
    void DrawHUD();

    // showFlash が false なら、画面全体を赤くする被弾の色を出さない
    void TakeDamage(int damage, bool showFlash = true);
    // 燃えている間は、ゲージを赤く点滅させる
    void SetBurning(bool isBurning) { isBurning_ = isBurning; }
    void Refill();

    bool IsDead() const { return currentHP_ <= 0; }
    int GetCurrent() const { return currentHP_; }
    // スタミナのゲージを真下に並べるために使う
    const FS::Vector2& GetGaugeOffset() const { return gaugeOffset_; }
    const FS::Vector2& GetGaugeSize() const { return gaugeSize_; }

private:
    int maxHP_ = 100;
    int currentHP_ = 100;

    // 画面左下のゲージ
    std::unique_ptr<FS::Sprite> gaugeBack_;
    std::unique_ptr<FS::Sprite> gaugeFill_;
    FS::Vector2 gaugeOffset_ = { 40.0f, -40.0f };   // 画面左下からのずれ
    FS::Vector2 gaugeSize_ = { 300.0f, 10.0f };
    FS::Vector4 gaugeColor_ = { 0.35f, 0.8f, 0.3f, 0.95f };

    // 被弾時に画面全体を赤くする
    std::unique_ptr<FS::Sprite> damageFlash_;
    FS::Vector4 flashColor_ = { 0.8f, 0.0f, 0.0f, 0.35f };
    float flashDuration_ = 0.35f;
    float flashTimer_ = 0.0f;

    // 燃えている間のゲージの点滅
    bool isBurning_ = false;
    float burnBlinkTime_ = 0.0f;
    FS::Vector4 burnBlinkColor_ = { 1.0f, 0.3f, 0.05f, 0.95f };
    float burnBlinkSpeed_ = 8.0f;
};
