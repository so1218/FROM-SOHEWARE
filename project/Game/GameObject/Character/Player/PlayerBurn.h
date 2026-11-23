#pragma once
#include "Engine.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"
#include "BurnStatus.h"

namespace FS
{
class AnimationModel;
}

class PlayerBurn
{
public:
    explicit PlayerBurn(FS::Engine* engine);

    void BindProperties(FS::PropertyBinder& binder);
    void DrawProperties(FS::PropertyBinder& binder);

    // 足元の炎で燃え方を進め、このフレームに受けるダメージを返す
    int Update(const FS::Vector3& footPosition, const FS::AnimationModel* bodyModel, bool canTakeDamage);
    // ダメージが入った瞬間の光と音
    void OnDamaged();
    // 燃えていないことにして、演出も消す
    void Reset();

    bool IsBurning() const { return status_.IsBurning(); }

    // ダメージが入った瞬間に、パッドを振動させ、カメラを揺らす強さ
    float GetVibrationStrength() const { return vibrationStrength_; }
    float GetVibrationTime() const { return vibrationTime_; }
    float GetCameraShake() const { return cameraShake_; }

private:
    void UpdateEmbers(const FS::AnimationModel* bodyModel);
    void ApplyVignette(float deltaTime);

    FS::Engine* engine_ = nullptr;
    BurnStatus status_;
    BurnConfig config_;
    FS::ParticleEmitter* embersEmitter_ = nullptr;
    bool isEmberPlaying_ = false;

    float effectRatio_ = 0.0f;   
    float flashTimer_ = 0.0f;
    float elapsedTime_ = 0.0f;

    // 画面の縁
    float vignetteAmount_ = 0.5f;
    float flickerAmount_ = 0.15f;        
    float flickerSpeed_ = 9.0f;
    FS::Vector3 vignetteColor_ = { 1.0f, 0.35f, 0.05f };      
    FS::Vector3 vignetteHotColor_ = { 0.85f, 0.05f, 0.02f };   
    float vignetteStart_ = 0.3f;
    float vignetteEnd_ = 0.95f;
    float fadeInTime_ = 0.2f;
    float fadeOutTime_ = 0.6f;

    // ダメージが入った瞬間
    float flashAmount_ = 0.35f;          
    float flashTime_ = 0.25f;
    float vibrationStrength_ = 0.25f;
    float vibrationTime_ = 0.1f;
    float cameraShake_ = 0.012f;
    float burnSoundVolume_ = 0.5f;      
    float voiceVolume_ = 0.5f;           
};
