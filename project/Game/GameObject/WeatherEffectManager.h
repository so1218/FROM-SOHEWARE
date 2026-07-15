#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include "Terrain.h"
#include "PropertyBinder.h"

class WeatherEffectManager : public FE::GameObject
{
public:
    WeatherEffectManager(FE::Engine* engine, FE::Camera* camera, Player* player, FE::Terrain* terrain);

    void Initialize() override;
    void Update() override;
    void DebugDraw() override;

private:

    FE::Engine* engine_;
    FE::Camera* camera_;
    Player* player_;
    FE::Terrain* terrain_;

    std::unique_ptr<FE::PropertyBinder> binder_ = nullptr;

    // エフェクトのポインタを保持
    std::unique_ptr<FE::ParticleEmitter> rainParticleEmitter_ = nullptr;
    FE::ParticleEmitter* rainParticleEmitterPtr_;
    std::unique_ptr<FE::ParticleEmitter> snowParticleEmitter_ = nullptr;
    FE::ParticleEmitter* snowParticleEmitterPtr_;
    std::unique_ptr<FE::ParticleEmitter> thunderParticleEmitter_ = nullptr;
    FE::ParticleEmitter* thunderParticleEmitterPtr_;

    // 雷雨用の制御タイマー
    float thunderIntervalTimer_ = 0.0f; 
    float flashTimer_ = 0.0f;           
    bool isFlashing_ = false;         

    float thunderMinInterval_ = 3.0f;    
    float thunderMaxInterval_ = 8.0f;  
    FE::Vector3 flashColor_ = { 0.9f, 0.95f, 1.0f }; 
    float flashDuration_ = 0.4f;      
    float maxFlashIntensity_ = 10.0f;  

    std::mt19937 randomEngine_;
};