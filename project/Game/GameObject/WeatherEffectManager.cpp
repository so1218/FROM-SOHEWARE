#include "pch.h"
#include "WeatherEffectManager.h"
#include "EnvironmentManager.h"
#include "PropertyBinder.h"

using namespace FE;

WeatherEffectManager::WeatherEffectManager(FE::Engine* engine, FE::Camera* camera, Player* player, Terrain* terrain)
{
    engine_ = engine;
    camera_ = camera;
    player_ = player;
    terrain_ = terrain;
}

void WeatherEffectManager::Initialize()
{
    rainParticleEmitter_ = engine_->GetParticleSystem()->CreateEmitter("rain");
    rainParticleEmitter_->SetTargetToFollow(&camera_->GetWorldTransform());
    rainParticleEmitterPtr_ = rainParticleEmitter_.get();
    engine_->GetParticleSystem()->AddEmitter(std::move(rainParticleEmitter_));

    snowParticleEmitter_ = engine_->GetParticleSystem()->CreateEmitter("snow");
    snowParticleEmitter_->SetTargetToFollow(&player_->GetTransform());
    snowParticleEmitterPtr_ = snowParticleEmitter_.get();
    engine_->GetParticleSystem()->AddEmitter(std::move(snowParticleEmitter_));
}

void WeatherEffectManager::Update()
{
    auto env = EnvironmentManager::GetInstance();
    WeatherState current = env->GetCurrentWeather();

    if (current == WeatherState::Rain || current == WeatherState::Thunderstorm)
    {
        rainParticleEmitterPtr_->Play();
        snowParticleEmitterPtr_->Stop();

        terrain_->GetMaterialData()->metalness = 0.9f;
        terrain_->GetMaterialData()->roughness = 0.25f;
        terrain_->GetMaterialData()->environmentMapIntensity = 0.05f;
        terrain_->GetMaterialData()->enableRipple = true;
        terrain_->GetMaterialData()->rippleScale = 0.4f;
        terrain_->GetMaterialData()->rippleStrength = 10.0f;
        terrain_->GetMaterialData()->rippleSpeed = 0.4f;
        terrain_->GetMaterialData()->rippleSize = 1.2f;
        terrain_->GetMaterialData()->rippleFrequency = 6.0f;
        terrain_->SetRippleTexture("normal_31");

        terrain_->GetMaterialData()->normalIntensity = 1.7f;
        terrain_->GetMaterialData()->color = { 94.0f / 255.0f,165.0f / 255.0f,86.0f / 255.0f,1.0f };
        terrain_->GetMaterialData()->emissiveIntensity = 12.0f;
    }
    else if (current == WeatherState::Snow)
    {
        snowParticleEmitterPtr_->Play();
        rainParticleEmitterPtr_->Stop();

        terrain_->GetMaterialData()->metalness = 0.13f;
        terrain_->GetMaterialData()->roughness = 1.00f;
        terrain_->GetMaterialData()->environmentMapIntensity = 0.2f;
        terrain_->GetMaterialData()->enableRipple = false;
        terrain_->GetMaterialData()->normalIntensity = 0.2f;
        terrain_->GetMaterialData()->color = { 1.0f,1.0f,1.0f,1.0f };
        terrain_->GetMaterialData()->emissiveIntensity = 10.0f;
    }
    else
    {
        rainParticleEmitterPtr_->Stop();
        snowParticleEmitterPtr_->Stop();

        terrain_->GetMaterialData()->metalness = 0.15f;
        terrain_->GetMaterialData()->roughness = 1.00f;
        terrain_->GetMaterialData()->environmentMapIntensity = 0.0f;
        terrain_->GetMaterialData()->enableRipple = false;
        terrain_->GetMaterialData()->normalIntensity = 1.7f;
        terrain_->GetMaterialData()->color = { 94.0f / 255.0f,165.0f / 255.0f,86.0f / 255.0f,1.0f };
        terrain_->GetMaterialData()->emissiveIntensity = 6.0f;
    }

}