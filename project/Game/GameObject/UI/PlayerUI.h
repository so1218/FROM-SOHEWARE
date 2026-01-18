#pragma once
#include "GameObject.h"
#include "Player.h"
#include "Sprite.h"

class PlayerUI : public GameObject
{
public:
    PlayerUI(Engine* engine, Player* player);

    GameObjectType GetType() const override { return GameObjectType::UI; }

    void Initialize() override;

    void Update() override;

    void Draw() override;

    void DebugDraw() override;

    void ApplyGlobalVariables();
    std::vector<std::string> GetGlobalVariableGroupName() const { return { "PlayerUI" }; }

private:
    Player* player_ = nullptr; 

    std::unique_ptr<Sprite> xpBarBgSprite_;
    std::unique_ptr<Sprite> xpBarSprite_;

    std::unique_ptr<Sprite> hpBarBgSprite_;
    std::unique_ptr<Sprite> hpBarSprite_;

    // 数字テクスチャのハンドル配列
    std::array<uint32_t, 10> digitTextureId_;

    // 現在表示しているレベルの数値
    int currentDisplayLevel_ = -1;

    // 数字描画用のスプライトリスト
    std::vector<std::unique_ptr<Sprite>> levelNumberSprites_;

    // XPバー設定
    Vector2 xpBarPos_ = { (1280.0f - 800.0f) / 2.0f, 10.0f };
    Vector2 xpBarSize_ = { 800.0f, 20.0f };

    // HPバー設定
    Vector2 hpBarSize_ = { 100.0f, 10.0f };
    float hpBarOffsetHeight_ = -1.5f;

    // レベル数字設定
    Vector2 levelNumberPos_ = { 20.0f, 10.0f };
    float numberSpace_ = 24.0f;
    Vector2 numberSize_ = { 32.0f, 32.0f };

    std::unique_ptr<Sprite> spriteMove_;
    std::unique_ptr<Sprite> spriteCamera_;
    std::unique_ptr<Sprite> spriteExpFrame_;
    std::unique_ptr<Sprite> spriteHpFrame_;
    std::unique_ptr<Sprite> spriteIkinokore_;

    Vector2 spriteSizeMove_ = { 640.0f, 360.0f };
    Vector2 spriteSizeCamera_ = { 640.0f, 360.0f };
    Vector2 spriteSizeExpFrame_ = { 640.0f, 360.0f };
    Vector2 spriteSizeHpFrame_ = { 640.0f, 360.0f };
    Vector2 spriteSizeIkinokore_ = { 640.0f, 360.0f };

    Vector2 spritePosMove_ = { 640.0f, 360.0f };
    Vector2 spritePosCamera_ = { 640.0f, 360.0f };
    Vector2 spritePosExpFrame_ = { 640.0f, 360.0f };
    Vector2 spritePosHpFrame_ = { 640.0f, 360.0f };
    Vector2 spritePosIkinokore_ = { 640.0f, 360.0f };
};