#pragma once
#include "PropertyBinder.h"
#include "Sprite.h"

// プレイヤーの HP。被弾すると画面が一瞬赤くなる
class PlayerHealth
{
public:
    PlayerHealth(FS::Engine* engine);

    void BindProperties(FS::PropertyBinder& binder);
    void DrawProperties(FS::PropertyBinder& binder);

    void Update();
    void DrawHUD();

    void TakeDamage(int damage);
    void Refill();

    bool IsDead() const { return currentHP_ <= 0; }
    int GetCurrent() const { return currentHP_; }

private:
    int maxHP_ = 100;
    int currentHP_ = 100;

    // 画面左下のゲージ
    std::unique_ptr<FS::Sprite> gaugeBack_;
    std::unique_ptr<FS::Sprite> gaugeFill_;
    FS::Vector2 gaugeOffset_ = { 40.0f, -40.0f };   // 画面左下からのずれ
    FS::Vector2 gaugeSize_ = { 300.0f, 10.0f };
    FS::Vector4 gaugeColor_ = { 0.9f, 0.9f, 0.9f, 0.9f };
    FS::Vector4 lowHealthColor_ = { 0.9f, 0.2f, 0.15f, 0.9f };
    float lowHealthRatio_ = 0.3f;   // この割合を下回るとゲージが赤くなる

    // 被弾時に画面全体を赤くする
    std::unique_ptr<FS::Sprite> damageFlash_;
    FS::Vector4 flashColor_ = { 0.8f, 0.0f, 0.0f, 0.35f };
    float flashDuration_ = 0.35f;
    float flashTimer_ = 0.0f;
};
