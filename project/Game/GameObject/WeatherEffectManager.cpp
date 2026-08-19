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

    thunderRainParticleEmitter_ = engine_->GetParticleSystem()->CreateEmitter("thunderRain");
    thunderRainParticleEmitter_->SetTargetToFollow(&player_->GetTransform());
    thunderRainParticleEmitterPtr_ = thunderRainParticleEmitter_.get();
    engine_->GetParticleSystem()->AddEmitter(std::move(thunderRainParticleEmitter_));

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
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    WeatherState current = env->GetCurrentWeather();
    WeatherState target = env->GetTargetWeather();
    float t = env->GetWeatherTransitionProgress();
    const WeatherProfile& profile = env->GetCurrentWeatherProfile();
    float wetness = profile.wetness;

    // === パラメータの取得とブレンド ===
    WeatherVisualParams currentP = GetWeatherVisualParams(current);
    WeatherVisualParams targetP = GetWeatherVisualParams(target);

    // Terrainのマテリアル設定適用
    auto* mat = terrain_->GetMaterialData();
   /* mat->metalness = FE::Math::Lerp(currentP.metalness, targetP.metalness, t);
    mat->roughness = FE::Math::Lerp(currentP.roughness, targetP.roughness, t);*/
   /* mat->environmentMapIntensity = FE::Math::Lerp(currentP.environmentMapIntensity, targetP.environmentMapIntensity, t);*/
    mat->rippleSize = FE::Math::Lerp(currentP.rippleSize, targetP.rippleSize, t);
    //mat->normalIntensity = FE::Math::Lerp(currentP.normalIntensity, targetP.normalIntensity, t);
    //mat->emissiveIntensity = FE::Math::Lerp(currentP.emissiveIntensity, targetP.emissiveIntensity, t);

    //mat->color = {
    //    FE::Math::Lerp(currentP.color.x, targetP.color.x, t),
    //    FE::Math::Lerp(currentP.color.y, targetP.color.y, t),
    //    FE::Math::Lerp(currentP.color.z, targetP.color.z, t),
    //    1.0f
    //};

    // bool値や固定値の設定
    mat->enableRipple = (wetness > 0.1f);
    mat->rippleScale = 0.4f;
    mat->rippleStrength = 10.0f;
    mat->rippleSpeed = 0.4f;
    mat->rippleFrequency = 6.0f;
    terrain_->SetRippleTexture("normal_31");

    // Volumetric Fogの設定適用
    auto* fog = engine_->GetPostEffectManager()->GetVolumetricFogSettings();
    fog->scatteringIntensity = FE::Math::Lerp(currentP.scatteringIntensity, targetP.scatteringIntensity, t);
    fog->noiseScale = FE::Math::Lerp(currentP.noiseScale, targetP.noiseScale, t);
    fog->noiseIntensity = targetP.noiseIntensity;
    fog->heightDensity = targetP.heightDensity;
    fog->heightFalloff = targetP.heightFalloff;
    fog->extinction = FE::Math::Lerp(currentP.extinction, targetP.extinction, t);
    fog->erosion = FE::Math::Lerp(currentP.erosion, targetP.erosion, t);
    fog->windSpeed = targetP.windSpeed;

    fog->ambientLight = {
        FE::Math::Lerp(currentP.ambientLight.x, targetP.ambientLight.x, t),
        FE::Math::Lerp(currentP.ambientLight.y, targetP.ambientLight.y, t),
        FE::Math::Lerp(currentP.ambientLight.z, targetP.ambientLight.z, t)
    };

    fog->windDirection = targetP.windDirection;
    // === パーティクルと雷の制御（ON/OFFやタイマー） ===

    // 各天候の「パーティクルの強さ (0.0 ～ 1.0)」を計算
    auto CalculateIntensity = [&](WeatherState checkState) -> float {
        bool isTarget = (target == checkState);
        bool isCurrent = (current == checkState);

        if (isTarget && !isCurrent) return t;           // 降り始め（徐々に強く）
        if (!isTarget && isCurrent) return 1.0f - t;    // 止み始め（徐々に弱く）
        if (isTarget && isCurrent)  return 1.0f;        // 完全に降っている
        return 0.0f;                                    // 降っていない
        };

    float rainIntensity = CalculateIntensity(WeatherState::Rain);
    float thunderRainIntensity = CalculateIntensity(WeatherState::Thunderstorm);
    float snowIntensity = CalculateIntensity(WeatherState::Snow);

    // 雨パーティクルの制御
    if (rainIntensity > 0.0f) {
        rainParticleEmitterPtr_->Play();
        rainParticleEmitterPtr_->SetEmissionRateMultiplier(rainIntensity);
    }
    else {
        rainParticleEmitterPtr_->Stop();
    }

    // 雷雨パーティクルの制御
    if (thunderRainIntensity > 0.0f) {
        thunderRainParticleEmitterPtr_->Play();
        thunderRainParticleEmitterPtr_->SetEmissionRateMultiplier(thunderRainIntensity);
    }
    else {
        thunderRainParticleEmitterPtr_->Stop();
    }

    // 雪パーティクルの制御
    if (snowIntensity > 0.0f) {
        snowParticleEmitterPtr_->Play();
        snowParticleEmitterPtr_->SetEmissionRateMultiplier(snowIntensity);
    }
    else {
        snowParticleEmitterPtr_->Stop();
    }

    // 落雷の制御
    if (lightningSystem_) {
        lightningSystem_->Update();
    }

    // 雷は現在が雷雨または雷雨へ遷移中（t が0.5以上）で発生
    if (current == WeatherState::Thunderstorm || (target == WeatherState::Thunderstorm && t > 0.5f))
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

    if (ImGui::CollapsingHeader("雷発生設定", ImGuiTreeNodeFlags_DefaultOpen))
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

inline WeatherVisualParams GetWeatherVisualParams(FE::WeatherState state)
{
    WeatherVisualParams p;

    switch (state)
    {
    case FE::WeatherState::Rain:
    case FE::WeatherState::Thunderstorm:
        // Terrain
        p.metalness = 0.9f;
        p.roughness = 0.25f;
        p.environmentMapIntensity = 0.05f;
        p.rippleSize = 1.2f;
        p.normalIntensity = 1.7f;
        p.color = { 175.0f / 255.0f, 255.0f / 255.0f, 166.0f / 255.0f, 1.0f };
        p.emissiveIntensity = 12.0f;
        // Fog
        p.scatteringIntensity = 1.5f;
        p.noiseScale = 0.03f;
        p.noiseIntensity = 1.0f;
        p.heightDensity = 1.0f;
        p.heightFalloff = 0.3f;
        p.ambientLight = { 40.0f / 255.0f, 70.0f / 255.0f, 160.0f / 255.0f };
        p.extinction = 0.02f;
        p.erosion = 0.4f;
        p.windSpeed = 0.15f;
        p.windDirection = { 1.0f, 1.0f, 1.0f };
        break;

    case FE::WeatherState::Snow:
        // Terrain
        p.metalness = 0.13f;
        p.roughness = 1.00f;
        p.environmentMapIntensity = 0.0f;
        p.rippleSize = 0.0f;
        p.normalIntensity = 0.2f;
        p.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        p.emissiveIntensity = 10.0f;
        // Fog
        p.scatteringIntensity = 1.5f;
        p.noiseScale = 0.03f;
        p.noiseIntensity = 1.0f;
        p.heightDensity = 0.1f;
        p.heightFalloff = 0.0f;
        p.ambientLight = { 85.0f / 255.0f, 110.0f / 255.0f, 190.0f / 255.0f };
        p.extinction = 0.005f;
        p.erosion = 0.8f;
        p.windSpeed = 0.3f;
        p.windDirection = { 1.0f, 0.0f, 0.5f };
        break;

    default: // Sunny, Cloudy
        // Terrain
        p.metalness = 0.15f;
        p.roughness = 1.00f;
        p.environmentMapIntensity = 0.5f;
        p.rippleSize = 0.0f;
        p.normalIntensity = 1.7f;
        p.color = { 175.0f / 255.0f, 255.0f / 255.0f, 166.0f / 255.0f, 1.0f };
        p.emissiveIntensity = 3.5f;
        // Fog
        p.scatteringIntensity = 10.0f;
        p.noiseScale = 0.03f; // 晴れでもスケールは維持しておくと遷移が綺麗
        p.noiseIntensity = 0.0f;
        p.heightDensity = 0.0f;
        p.heightFalloff = 0.0f;
        p.ambientLight = { 10.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f };
        p.extinction = 0.005f;
        p.erosion = 0.0f;
        p.windSpeed = 0.1f;
        p.windDirection = { 1.0f, 1.0f, 1.0f };
        break;
    }

    return p;
}