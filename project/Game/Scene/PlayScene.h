#pragma once
#include "BaseScene.h"
#include "Player.h"
#include "ParticleEmitter.h"
#include "Enemy.h"
#include "FollowCamera.h"
#include "Sprite.h"

class PlayScene : public BaseScene
{
public:
    PlayScene(Engine* engine, Camera* camera);

    ~PlayScene();

    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    // メンバー変数
    std::unique_ptr<Player> player_;
    std::unique_ptr<Enemy> enemy_;

    std::unique_ptr<ParticleEmitter> hanabi1Emitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> hanabi2Emitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> hanabi3Emitter_ = nullptr;
    std::unique_ptr<ParticleEmitter> clearEmitter_ = nullptr;

    std::unique_ptr<Sprite> sprite_;
    std::unique_ptr<Sprite> spriteUse_;

    Vector2 spriteSize_ = { 640.0f, 360.0f };
    Vector2 spriteSizeUse_ = { 640.0f, 360.0f };

    Vector2 spritePos_ = { 640.0f, 360.0f };
    Vector2 spritePosUse_ = { 640.0f, 360.0f };
};

