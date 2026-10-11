#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include "Terrain.h"
#include "PropertyBinder.h"
#include "LightningSystem.h"
#include "EnvironmentManager.h"
#include "CameraManager.h"

class WeatherEffectManager : public FE::GameObject
{
public:
    WeatherEffectManager(FE::Engine* engine, FE::Camera* camera, Player* player, FE::Terrain* terrain);

    void Initialize() override;
    void Update() override;
    void DebugDraw() override;

    void SetCameraManager(FE::CameraManager* cameraManager) { cameraManager_ = cameraManager; }

private:

    FE::Engine* engine_;
    FE::Camera* camera_;
    Player* player_;
    FE::Terrain* terrain_;

    std::unique_ptr<FE::PropertyBinder> binder_ = nullptr;

    // エフェクトのポインタを保持
    std::unique_ptr<FE::ParticleEmitter> rainParticleEmitter_ = nullptr;
    FE::ParticleEmitter* rainParticleEmitterPtr_;
    std::unique_ptr<FE::ParticleEmitter> thunderRainParticleEmitter_ = nullptr;
    FE::ParticleEmitter* thunderRainParticleEmitterPtr_;
    std::unique_ptr<FE::ParticleEmitter> thunderStrikeParticleEmitter_ = nullptr;
    FE::ParticleEmitter* thunderStrikeParticleEmitterPtr_;

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

    std::unique_ptr<FE::LightningSystem> lightningSystem_;

    // 落雷の発生範囲
    float strikeRadiusMin_ = 30.0f;
    float strikeRadiusMax_ = 100.0f;
    float strikeHeight_ = 250.0f;  // 空の高さ

    // カメラシェイク用パラメータ
    float thunderShakeDuration_ = 0.5f;
    float thunderShakeIntensity_ = 2.0f;
    float thunderShakeMaxDistance_ = 300.0f;

    FE::CameraManager* cameraManager_ = nullptr;

    // 現在再生中のBGMの名前
    std::string currentBgmName_ = "";
};

struct WeatherVisualParams {
    // Terrain
    float metalness;
    float roughness;
    float environmentMapIntensity;
    float rippleSize;
    float normalIntensity;
    FE::Vector4 color;
    float emissiveIntensity;

    // Volumetric Fog
    float scatteringIntensity;
    float noiseScale;
    float noiseIntensity;
    float heightDensity;
    float heightFalloff;
    FE::Vector3 ambientLight;
    float extinction;
    float erosion;
    float windSpeed;
    FE::Vector3 windDirection;
};