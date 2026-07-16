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

    lightningSystem_ = std::make_unique<LightningSystem>(engine_);
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

    lightningSystem_->Initialize();

    binder_->Bind("ThunderMinInterval", &thunderMinInterval_, 3.0f, 0.1f, 0.5f, 30.0f);
    binder_->Bind("ThunderMaxInterval", &thunderMaxInterval_, 8.0f, 0.1f, 1.0f, 60.0f);
    binder_->Bind("StrikeRadiusMin", &strikeRadiusMin_, 30.0f, 1.0f, 5.0f, 100.0f);
    binder_->Bind("StrikeRadiusMax", &strikeRadiusMax_, 100.0f, 1.0f, 30.0f, 500.0f);
    binder_->Bind("StrikeHeight", &strikeHeight_, 250.0f, 5.0f, 50.0f, 1000.0f);
}

void WeatherEffectManager::Update()
{
    auto env = EnvironmentManager::GetInstance();
    WeatherState current = env->GetCurrentWeather();

    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    if (lightningSystem_)
    {
        lightningSystem_->Update();
    }

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
            // プレイヤーの位置を中心にランダムな発生座標を計算
            Vector3 playerPos = player_->GetTransform().translation_;

            // 角度(0～360度)と距離(Min～Max)をランダムに決定
            std::uniform_real_distribution<float> distAngle(0.0f, 3.14159265f * 2.0f);
            std::uniform_real_distribution<float> distRadius(strikeRadiusMin_, strikeRadiusMax_);

            float angle = distAngle(randomEngine_);
            float radius = distRadius(randomEngine_);

            float targetX = playerPos.x + std::cos(angle) * radius;
            float targetZ = playerPos.z + std::sin(angle) * radius;

            // 地面の高さを取得
            float groundY = terrain_->GetHeight(targetX, targetZ); 

            // 発生地点（上空）と、目標地点（地面）
            Vector3 startPos(targetX, playerPos.y + strikeHeight_, targetZ);
            Vector3 endPos(targetX, groundY, targetZ);

            // 雷を生成
            lightningSystem_->SpawnStrike(startPos, endPos);

            // 次の雷までの時間を再設定
            std::uniform_real_distribution<float> dist(thunderMinInterval_, thunderMaxInterval_);
            thunderIntervalTimer_ = dist(randomEngine_);
        }
    }
}

void WeatherEffectManager::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("環境設定");

    if (ImGui::CollapsingHeader("雷（Thunderstorm）発生設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("ThunderMinInterval", "最小インターバル (秒)");
        binder_->Draw("ThunderMaxInterval", "最大インターバル (秒)");

        // 最小値が最大値を超えないように調整する安全柵
        if (thunderMinInterval_ > thunderMaxInterval_) {
            thunderMaxInterval_ = thunderMinInterval_;
        }

        ImGui::Separator();
        ImGui::Text("落雷の発生範囲");
        binder_->Draw("StrikeRadiusMin", "最小発生距離");
        binder_->Draw("StrikeRadiusMax", "最大発生距離");
        binder_->Draw("StrikeHeight", "雷雲の高さ(Y)");

        if (strikeRadiusMin_ > strikeRadiusMax_) {
            strikeRadiusMax_ = strikeRadiusMin_;
        }
    }

    ImGui::End();
#endif

    if (lightningSystem_)
    {
        lightningSystem_->DebugDraw();
    }
}

