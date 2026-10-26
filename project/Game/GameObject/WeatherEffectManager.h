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

class WeatherEffectManager : public FS::GameObject
{
public:
    WeatherEffectManager(FS::Engine* engine, FS::Camera* camera, Player* player, FS::Terrain* terrain);

    void Initialize() override;
    void Update() override;
    void DebugDraw() override;

    void SetCameraManager(FS::CameraManager* cameraManager) { cameraManager_ = cameraManager; }

private:

    FS::Engine* engine_;
    FS::Camera* camera_;
    Player* player_;
    FS::Terrain* terrain_;

    std::unique_ptr<FS::PropertyBinder> binder_ = nullptr;

    // エフェクトのポインタを保持
    std::unique_ptr<FS::ParticleEmitter> rainParticleEmitter_ = nullptr;
    FS::ParticleEmitter* rainParticleEmitterPtr_;
    std::unique_ptr<FS::ParticleEmitter> thunderRainParticleEmitter_ = nullptr;
    FS::ParticleEmitter* thunderRainParticleEmitterPtr_;
    std::unique_ptr<FS::ParticleEmitter> thunderStrikeParticleEmitter_ = nullptr;
    FS::ParticleEmitter* thunderStrikeParticleEmitterPtr_;

    // 雷雨用の制御タイマー
    float thunderIntervalTimer_ = 0.0f; 
    float flashTimer_ = 0.0f;           
    bool isFlashing_ = false;         

    float thunderMinInterval_ = 3.0f;    
    float thunderMaxInterval_ = 8.0f;  
    FS::Vector3 flashColor_ = { 0.9f, 0.95f, 1.0f }; 
    float flashDuration_ = 0.4f;      
    float maxFlashIntensity_ = 10.0f;  

    std::mt19937 randomEngine_;

    std::unique_ptr<FS::LightningSystem> lightningSystem_;

    // 落雷の発生範囲
    float strikeRadiusMin_ = 30.0f;
    float strikeRadiusMax_ = 100.0f;
    float strikeHeight_ = 250.0f;  // 空の高さ

    // カメラシェイク用パラメータ
    float thunderShakeDuration_ = 0.5f;
    float thunderShakeIntensity_ = 2.0f;
    float thunderShakeMaxDistance_ = 300.0f;

    FS::CameraManager* cameraManager_ = nullptr;

    // 現在再生中のBGMの名前
    std::string currentBgmName_ = "";
};

struct WeatherVisualParams
{
    // Terrain
    float metalness;
    float roughness;
    float environmentMapIntensity;
    float rippleSize;
    float normalIntensity;
    FS::Vector4 color;
    float emissiveIntensity;

    // Volumetric Fog
    float scatteringIntensity;
    float noiseScale;
    float noiseIntensity;
    float heightDensity;
    float heightFalloff;
    FS::Vector3 ambientLight;
    float extinction;
    float erosion;
    float windSpeed;
    FS::Vector3 windDirection;
};