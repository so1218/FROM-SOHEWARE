#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "ParticleEmitter.h"

class TitleScene : public BaseScene
{
public:
    TitleScene(Engine* engine);

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
    std::unique_ptr<ParticleEmitter> titleEmitter_ = nullptr;

    std::unique_ptr<Sprite> sprite_;
    std::unique_ptr<Sprite> spriteUse_;
    std::unique_ptr<Sprite> spritePress_;

    Vector2 spriteSize_ = { 640.0f, 360.0f };
    Vector2 spriteSizeUse_ = { 640.0f, 360.0f };
    Vector2 spriteSizePress_ = { 640.0f, 360.0f };

    Vector2 spritePos_ = { 640.0f, 360.0f };
    Vector2 spritePosUse_ = { 640.0f, 360.0f };
    Vector2 spritePosPress_ = { 640.0f, 360.0f };
};
