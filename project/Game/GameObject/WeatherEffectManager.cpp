#include "pch.h"
#include "WeatherEffectManager.h"
#include "EnvironmentManager.h"
#include "TimeManager.h"

using namespace FE;

WeatherEffectManager::WeatherEffectManager(FE::Engine* engine, FE::Camera* camera, Player* player, Terrain* terrain)
{
    engine_ = engine;
    camera_ = camera;
    player_ = player;
    terrain_ = terrain;

    std::random_device seedGen;
    randomEngine_.seed(seedGen());

    binder_ = std::make_unique<FE::PropertyBinder>(engine_, "WeatherEffect");
}

void WeatherEffectManager::Initialize()
{
    rainParticleEmitter_ = engine_->GetParticleSystem()->CreateEmitter("rain");
    rainParticleEmitter_->SetTargetToFollow(&player_->GetTransform());
    rainParticleEmitterPtr_ = rainParticleEmitter_.get();
    engine_->GetParticleSystem()->AddEmitter(std::move(rainParticleEmitter_));

    snowParticleEmitter_ = engine_->GetParticleSystem()->CreateEmitter("snow");
    snowParticleEmitter_->SetTargetToFollow(&player_->GetTransform());
    snowParticleEmitterPtr_ = snowParticleEmitter_.get();
    engine_->GetParticleSystem()->AddEmitter(std::move(snowParticleEmitter_));

    thunderParticleEmitter_ = engine_->GetParticleSystem()->CreateEmitter("thunder");
    thunderParticleEmitter_->SetTargetToFollow(&player_->GetTransform());
    thunderParticleEmitterPtr_ = thunderParticleEmitter_.get();
    engine_->GetParticleSystem()->AddEmitter(std::move(thunderParticleEmitter_));

    binder_->Bind("ThunderMinInterval", &thunderMinInterval_, 3.0f, 0.1f, 0.5f, 30.0f);
    binder_->Bind("ThunderMaxInterval", &thunderMaxInterval_, 8.0f, 0.1f, 1.0f, 60.0f);
    binder_->BindColor("FlashColor", &flashColor_, { 0.9f, 0.95f, 1.0f });
    binder_->Bind("FlashDuration", &flashDuration_, 0.4f, 0.05f, 0.05f, 3.0f);
    binder_->Bind("MaxFlashIntensity", &maxFlashIntensity_, 10.0f, 0.5f, 0.0f, 100.0f);
}

void WeatherEffectManager::Update()
{
    auto env = EnvironmentManager::GetInstance();
    WeatherState current = env->GetCurrentWeather();

    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

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

        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->scatteringIntensity = 1.5f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->noiseScale = 0.03f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->noiseIntensity = 1.0f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->heightDensity = 1.0f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->heightFalloff = 0.3f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->ambientLight = { 85.0f / 255.0f,110.0f / 255.0f,190.0f / 255.0f };
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->extinction = 0.02f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->erosion = 0.4f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->windSpeed = 0.15f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->windDirection = { 1.0f,1.0f,1.0f };
    }
    else if (current == WeatherState::Snow)
    {
        snowParticleEmitterPtr_->Play();
        rainParticleEmitterPtr_->Stop();

        terrain_->GetMaterialData()->metalness = 0.13f;
        terrain_->GetMaterialData()->roughness = 1.00f;
        terrain_->GetMaterialData()->environmentMapIntensity = 0.0f;
        terrain_->GetMaterialData()->enableRipple = false;
        terrain_->GetMaterialData()->normalIntensity = 0.2f;
        terrain_->GetMaterialData()->color = { 1.0f,1.0f,1.0f,1.0f };
        terrain_->GetMaterialData()->emissiveIntensity = 10.0f;

        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->scatteringIntensity = 1.5f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->noiseScale = 0.03f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->noiseIntensity = 1.0f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->heightDensity = 0.1f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->heightFalloff = 0.0f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->ambientLight = { 85.0f / 255.0f,110.0f / 255.0f,190.0f / 255.0f };
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->extinction = 0.005f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->erosion = 0.8f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->windSpeed = 0.3f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->windDirection = { 1.0f,0.0f,0.5f };
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

        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->scatteringIntensity = 10.0f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->noiseIntensity = 0.0f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->heightDensity = 0.0f;
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->ambientLight = { 10.0f / 255.0f,10.0f / 255.0f,10.0f / 255.0f };
        engine_->GetPostEffectManager()->GetVolumetricFogSettings()->extinction = 0.005f;
    }

    if (current == WeatherState::Thunderstorm)
    {
        thunderIntervalTimer_ -= deltaTime;

        // タイマーが0以下になったら雷を落とす
        if (thunderIntervalTimer_ <= 0.0f)
        {
            thunderParticleEmitterPtr_->Play();
            isFlashing_ = true;                 
            flashTimer_ = 0.0f;               

            // 次の雷までの時間
            std::uniform_real_distribution<float> dist(thunderMinInterval_, thunderMaxInterval_);
            thunderIntervalTimer_ = dist(randomEngine_);
        }
    }
    else
    {
        // 雷雨以外の天候になったら雷を強制停止
        thunderParticleEmitterPtr_->Stop();
        isFlashing_ = false;
        flashTimer_ = 0.0f;
    }

    // 画面全体のフラッシュ
    auto globalConsts = engine_->GetGlobalConstants();

    if (isFlashing_)
    {
        flashTimer_ += deltaTime;

        if (flashTimer_ >= flashDuration_)
        {
            // フラッシュ終了
            isFlashing_ = false;
            globalConsts->SetLightningFlash({ 1.0f, 1.0f, 1.0f }, 0.0f);
        }
        else
        {
            float normalizedTime = flashTimer_ / flashDuration_;
            float currentIntensity = std::sin(normalizedTime * Math::PI) * maxFlashIntensity_;

            globalConsts->SetLightningFlash(flashColor_, currentIntensity);
        }
    }
    else
    {
        // フラッシュしていない時は確実に強さを 0.0 にしておく
        globalConsts->SetLightningFlash({ 1.0f, 1.0f, 1.0f }, 0.0f);
    }
}

void WeatherEffectManager::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("環境設定");

    if (ImGui::CollapsingHeader("雷（Thunderstorm）パラメータ", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("ThunderMinInterval", "最小インターバル (秒)");
        binder_->Draw("ThunderMaxInterval", "最大インターバル (秒)");

        // 最小値が最大値を超えないように調整する安全柵
        if (thunderMinInterval_ > thunderMaxInterval_) {
            thunderMaxInterval_ = thunderMinInterval_;
        }

        binder_->Draw("FlashColor", "フラッシュの色");
        binder_->Draw("FlashDuration", "フラッシュの時間 (秒)");
        binder_->Draw("MaxFlashIntensity", "フラッシュの最大強度");
    }

    ImGui::End();
#endif
}

