#include "pch.h"
#include "WeatherEffectManager.h"
#include "EnvironmentManager.h"
#include "TimeManager.h"
#include "AudioPlayer.h"

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

    thunderStrikeParticleEmitter_ = engine_->GetParticleSystem()->CreateEmitter("thunderStrike");
    thunderStrikeParticleEmitterPtr_ = thunderStrikeParticleEmitter_.get();
    engine_->GetParticleSystem()->AddEmitter(std::move(thunderStrikeParticleEmitter_));

    lightningSystem_->Initialize();

    binder_->Bind("ThunderMinInterval", &thunderMinInterval_, 3.0f, 0.1f, 0.5f, 30.0f);
    binder_->Bind("ThunderMaxInterval", &thunderMaxInterval_, 8.0f, 0.1f, 1.0f, 60.0f);
    binder_->Bind("StrikeRadiusMin", &strikeRadiusMin_, 30.0f, 1.0f, 5.0f, 100.0f);
    binder_->Bind("StrikeRadiusMax", &strikeRadiusMax_, 100.0f, 1.0f, 30.0f, 500.0f);
    binder_->Bind("StrikeHeight", &strikeHeight_, 250.0f, 5.0f, 50.0f, 1000.0f);

    binder_->Bind("ThunderShakeDuration", &thunderShakeDuration_, 0.5f, 0.0f, 0.1f, 5.0f);
    binder_->Bind("ThunderShakeIntensity", &thunderShakeIntensity_, 2.0f, 0.0f, 0.1f, 20.0f);
    binder_->Bind("ThunderShakeMaxDist", &thunderShakeMaxDistance_, 300.0f, 1.0f, 10.0f, 1000.0f);
}

void WeatherEffectManager::Update()
{
    auto env = EnvironmentManager::GetInstance();
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    WeatherState current = env->GetCurrentWeather();
    WeatherState target = env->GetTargetWeather();
    float t = env->GetWeatherTransitionProgress();
    const WeatherProfile& profile = env->GetCurrentWeatherProfile();

    // EnvironmentManagerから、今フレームの補間済みビジュアルパラメータを取得
    const FE::WeatherVisualParams& visual = env->GetCurrentVisualParams();

    // -------------------------------------------------------------
    // Terrainのマテリアル設定適用
    // -------------------------------------------------------------
    auto mat = terrain_->GetMaterialData();
    mat->metalness = visual.metalness;
    mat->roughness = visual.roughness;
    mat->environmentMapIntensity = visual.environmentMapIntensity;
    mat->rippleSize = visual.rippleSize;
    mat->normalIntensity = visual.normalIntensity;
    mat->emissiveIntensity = visual.emissiveIntensity;
    mat->color = visual.color;

    // 波紋の個別制御
    mat->enableRipple = (profile.wetness > 0.1f);
    mat->rippleScale = 0.4f;
    mat->rippleStrength = 10.0f;
    mat->rippleSpeed = 0.4f;
    mat->rippleFrequency = 6.0f;
    terrain_->SetRippleTexture("normal_31");

    // -------------------------------------------------------------
    // Volumetric Fogの設定適用
    // -------------------------------------------------------------
    auto fog = engine_->GetPostEffectManager()->GetVolumetricFogSettings();
    fog->scatteringIntensity = visual.scatteringIntensity;
    fog->noiseScale = visual.noiseScale;
    fog->noiseIntensity = visual.noiseIntensity;
    fog->heightDensity = visual.heightDensity;
    fog->heightFalloff = visual.heightFalloff;
    fog->extinction = visual.extinction;
    fog->erosion = visual.erosion;
    fog->ambientLight = visual.ambientLight;
    fog->windSpeedMultiplier = visual.windSpeedMultiplier;

    // -------------------------------------------------------------
    // 各天候のパーティクル強さを計算 & 制御
    // -------------------------------------------------------------
    auto CalculateIntensity = [&](WeatherState checkState) -> float
        {
            bool isTarget = (target == checkState);
            bool isCurrent = (current == checkState);

            if (isTarget && !isCurrent) return t;           // 降り始め（徐々に強く）
            if (!isTarget && isCurrent) return 1.0f - t;    // 止み始め（徐々に弱く）
            if (isTarget && isCurrent)  return 1.0f;        // 完全に降っている
            return 0.0f;                                    // 降っていない
        };

    float rainIntensity = CalculateIntensity(WeatherState::Rain);
    float thunderRainIntensity = CalculateIntensity(WeatherState::Thunderstorm);

    // 雨パーティクルの制御
    if (rainIntensity > 0.0f)
    {
        rainParticleEmitterPtr_->Play();
        rainParticleEmitterPtr_->SetEmissionRateMultiplier(rainIntensity);
    }
    else
    {
        rainParticleEmitterPtr_->Stop();
    }

    // 雷雨パーティクルの制御
    if (thunderRainIntensity > 0.0f)
    {
        thunderRainParticleEmitterPtr_->Play();
        thunderRainParticleEmitterPtr_->SetEmissionRateMultiplier(thunderRainIntensity);
    }
    else
    {
        thunderRainParticleEmitterPtr_->Stop();
    }

    std::string nextBgm = "";
    if (target == WeatherState::Sunny || target == WeatherState::Cloudy)
    {
        nextBgm = "sunnyBGM";
    }
    if (target == WeatherState::Rain)
    {
        nextBgm = "rainBGM";
    }
    if (target == WeatherState::Thunderstorm) 
    {
        nextBgm = "thunderStormBGM";
    }

    // 再生すべきBGMが、現在鳴っているものと違ったら切り替える
    if (currentBgmName_ != nextBgm)
    {
        if (!currentBgmName_.empty())
        {
            AudioPlayer::GetInstance().StopUnique(currentBgmName_);
        }

        if (!nextBgm.empty()) 
        {
            AudioPlayer::GetInstance().PlayUnique(nextBgm, true, 50);
        }

        currentBgmName_ = nextBgm;
    }

    // -------------------------------------------------------------
    // 落雷の制御
    // -------------------------------------------------------------
    if (lightningSystem_)
    {
        lightningSystem_->Update();
    }

    // 雷は現在が雷雨または雷雨へ遷移中（t が0.5以上）で発生
    if (current == WeatherState::Thunderstorm || (target == WeatherState::Thunderstorm && t > 0.5f))
    {
        thunderIntervalTimer_ -= deltaTime;

        // タイマーが0以下になったら雷を落とす
        if (thunderIntervalTimer_ <= 0.0f)
        {
            Vector3 playerPos = player_->GetTransform().translation_;

            std::uniform_real_distribution<float> distAngle(0.0f, Math::PI * 2.0f);
            std::uniform_real_distribution<float> distRadius(strikeRadiusMin_, strikeRadiusMax_);

            float angle = distAngle(randomEngine_);
            float radius = distRadius(randomEngine_);

            float targetX = playerPos.x + std::cos(angle) * radius;
            float targetZ = playerPos.z + std::sin(angle) * radius;

            float groundY = terrain_->GetHeight(targetX, targetZ);

            Vector3 startPos(targetX, playerPos.y + strikeHeight_, targetZ);
            Vector3 endPos(targetX, groundY, targetZ);

            lightningSystem_->SpawnStrike(startPos, endPos);

            if (thunderStrikeParticleEmitterPtr_)
            {
                thunderStrikeParticleEmitterPtr_->SetPosition(endPos);
                thunderStrikeParticleEmitterPtr_->Play();
            }

            // カメラシェイク
            if (cameraManager_)
            {
                // 距離による減衰を計算 
                float distanceAttenuation = 1.0f - (radius / thunderShakeMaxDistance_);
                distanceAttenuation = std::clamp(distanceAttenuation, 0.0f, 1.0f);

                // 減衰をかけた最終的な揺れの強さ
                float finalIntensity = thunderShakeIntensity_ * distanceAttenuation;

                // 揺れが 0 以上の時だけシェイク
                if (finalIntensity > 0.01f)
                {
                    cameraManager_->RequestShake(thunderShakeDuration_, finalIntensity);
                }
            }

            std::uniform_real_distribution<float> dist(thunderMinInterval_, thunderMaxInterval_);
            thunderIntervalTimer_ = dist(randomEngine_);

			AudioPlayer::GetInstance().Play("lightningStrike", false, 50);
        }
    }
}

void WeatherEffectManager::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::Begin("環境設定");

    if (ImGui::CollapsingHeader("雷発生設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("ThunderMinInterval", "最小インターバル (秒)");
        binder_->Draw("ThunderMaxInterval", "最大インターバル (秒)");

        if (thunderMinInterval_ > thunderMaxInterval_)
        {
            thunderMaxInterval_ = thunderMinInterval_;
        }

        ImGui::Separator();
        ImGui::Text("落雷の発生範囲");
        binder_->Draw("StrikeRadiusMin", "最小発生距離");
        binder_->Draw("StrikeRadiusMax", "最大発生距離");
        binder_->Draw("StrikeHeight", "雷雲の高さ(Y)");

        if (strikeRadiusMin_ > strikeRadiusMax_)
        {
            strikeRadiusMax_ = strikeRadiusMin_;
        }

        ImGui::Separator();
        ImGui::Text("落雷時のカメラシェイク");
        binder_->Draw("ThunderShakeDuration", "シェイク時間 ");
        binder_->Draw("ThunderShakeIntensity", "シェイク強度");
        binder_->Draw("ThunderShakeMaxDist", "揺れが届く最大距離");
    }

    ImGui::End();
#endif

    if (lightningSystem_)
    {
        lightningSystem_->DebugDraw();
    }
}
