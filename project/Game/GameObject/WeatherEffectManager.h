#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include "Terrain.h"

class WeatherEffectManager : public FE::GameObject
{
public:
    WeatherEffectManager(FE::Engine* engine, FE::Camera* camera, Player* player, FE::Terrain* terrain);

    void Initialize() override;
    void Update() override;

private:

    FE::Engine* engine_;
    FE::Camera* camera_;
    Player* player_;
    FE::Terrain* terrain_;

    // エフェクトのポインタを保持
    std::unique_ptr<FE::ParticleEmitter> rainParticleEmitter_ = nullptr;
    FE::ParticleEmitter* rainParticleEmitterPtr_;
    std::unique_ptr<FE::ParticleEmitter> snowParticleEmitter_ = nullptr;
    FE::ParticleEmitter* snowParticleEmitterPtr_;
};