#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"
#include "Terrain.h"

class GrassFireEffects : public FS::GameObject
{
public:
    GrassFireEffects(FS::Engine* engine, const FS::Terrain* terrain);
    ~GrassFireEffects() override;

    void Initialize() override;
    void Update() override;
    void Draw() override {}
    void DebugGui() override;

private:
    // 燃えている塊の中心
    struct FirePeak
    {
        FS::Vector3 position;
        float intensity = 0.0f;
    };

    void FindFirePeaks(const std::vector<float>& readback, uint32_t resolution);
    // 一番強く燃えている所の周りの火をまとめて、その中心に 1 つだけ光を置く
    void SubmitFireLight(const std::vector<float>& readback, uint32_t resolution);
    void UpdateEmbers();
    void UpdateFlamesAndSmoke(const std::vector<float>& readback, uint32_t resolution, float deltaTime);
    // マスの中のランダムな地面の位置
    FS::Vector3 CalculateRandomPositionInCell(uint32_t cellIndex, uint32_t resolution) const;
    void StopEmbers();
    // 燃えている塊の近くで、燃える音
    void UpdateFireSounds(float deltaTime);
    void StopFireSounds();

    FS::Engine* engine_ = nullptr;
    const FS::Terrain* terrain_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    std::vector<FirePeak> peaks_;
    std::vector<FS::ParticleEmitter*> emberEmitters_;
    std::vector<bool> isEmberPlaying_;
    float elapsedTime_ = 0.0f;

    // マスごとの、最後に燃えていた時刻と、次の粒までのたまり具合
    std::vector<float> cellLastBurningTime_;
    std::vector<float> flameCredits_;
    std::vector<float> smokeCredits_;

    // 今フレームに炎か煙を出すマス
    struct ActiveCell
    {
        uint32_t index = 0;
        float flameRate = 0.0f;
        float smokeRate = 0.0f;
    };
    std::vector<ActiveCell> activeCells_;
    int estimatedFlameCount_ = 0;
    int estimatedSmokeCount_ = 0;

    // 燃える音
    struct FireSound
    {
        int handle = -1;
        FS::Vector3 position;
        FS::Vector3 targetPosition;
        float volume = 0.0f;
        float targetVolume = 0.0f;
    };
    std::vector<FireSound> fireSounds_;

    // 燃えている塊の見つけ方
    float peakThreshold_ = 0.12f;     
    float peakSeparation_ = 10.0f;    

    // 炎の光
    FS::Vector4 lightColor_ = { 1.0f, 0.45f, 0.12f, 1.0f };
    float lightIntensity_ = 12.0f;
    float lightRadius_ = 14.0f;
    float lightHeight_ = 1.5f;
    float lightVolumetricIntensity_ = 2.0f;
    float lightGatherRadius_ = 40.0f;      // 一番強い所からこの距離までの火をまとめる
    float lightSpreadRadiusRate_ = 1.5f;   // 火が広がっているほど、光の届く距離を伸ばす

    // 火の粉
    int maxEmberEmitterCount_ = 6;

    // 炎
    float flameThreshold_ = 0.05f;    
    float flameRatePerSquareMeter_ = 0.4f; 
    float flameSpawnHeight_ = 0.3f;  
    int maxFlameCount_ = 1200;        

    // 煙
    float smokeRatePerSquareMeter_ = 0.04f;
    float smokeDuration_ = 12.0f;
    float smokeSpawnHeight_ = 1.0f;    
    int maxSmokeCount_ = 400;

    // 燃える音
    int maxFireSoundCount_ = 3;      
    float fireSoundVolume_ = 0.8f;   
    float fireSoundMaxDistance_ = 50.0f;
    float fireSoundFadeTime_ = 1.0f;   
    float fireSoundHeight_ = 1.0f;     
};
