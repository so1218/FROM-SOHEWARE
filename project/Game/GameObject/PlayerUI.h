#pragma once
#include "GameObject.h"
#include "Player.h"
#include "Sprite.h"

class PlayerUI : public GameObject
{
public:
    PlayerUI(Engine* engine, Player* player) : engine_(engine), player_(player) {}

    GameObjectType GetType() const override { return GameObjectType::UI; }

    void Initialize() override;

    void Update() override;

    void Draw() override;

private:
    Engine* engine_ = nullptr;
    Player* player_ = nullptr; 

    std::unique_ptr<Sprite> xpBarBgSprite_;
    std::unique_ptr<Sprite> xpBarSprite_;

    std::unique_ptr<Sprite> hpBarBgSprite_;
    std::unique_ptr<Sprite> hpBarSprite_;

    const Vector2 kHpBarSize_ = { 100.0f, 10.0f }; 
    const float kHpBarOffsetHeight_ = -1.5f;

    // 数字テクスチャのハンドル配列
    std::array<uint32_t, 10> digitTextureHandles_;

    // 現在表示しているレベルの数値
    int currentDisplayLevel_ = -1;

    // 数字描画用のスプライトリスト
    std::vector<std::unique_ptr<Sprite>> levelNumberSprites_;

    // 数字の表示設定
    const Vector2 kLevelNumberPos_ = { 20.0f, 10.0f }; // 表示開始位置（画面左上など）
    const float kNumberSpace_ = 24.0f; // 数字ごとの間隔（画像の横幅に合わせて調整）
    const Vector2 kNumberSize_ = { 32.0f, 32.0f };     // 数字画像の表示サイズ
};