#include "pch.h"
#include "EnvironmentManager.h"
#include "TimeManager.h"
#include "LightManager.h"
#include "Engine.h"
#include "PropertyBinder.h"

namespace FE
{

EnvironmentManager::EnvironmentManager() = default;
EnvironmentManager::~EnvironmentManager() = default;

void EnvironmentManager::Initialize(Engine* engine)
{
    constantBuffer_ = BufferManager::CreateMappedConstantBuffer(
        engine->GetGraphicsDevice()->GetDevice(),
        &cbData_
    );

    if (!binder_)
    {
        binder_ = std::make_unique<PropertyBinder>(engine, "Environment");

        // 時間進行パラメータ
        binder_->Bind("TimeOfDay", &timeOfDay_, 12.0f, 0.1f, 0.0f, 24.0f);
        binder_->Bind("TimeSpeedMultiplier", &timeSpeedMultiplier_, 24.0f, 1.0f, -100.0f, 1000.0f);

        // -------------------------------------------------------------
        // 時間帯プロファイル
        // -------------------------------------------------------------
        binder_->BindColor("Night_ZenithColor", &profileNight_.zenithColor, { 0.05f, 0.15f, 0.4f });
        binder_->BindColor("Night_HorizonColor", &profileNight_.horizonColor, { 0.4f, 0.6f, 0.8f });
        binder_->BindColor("Night_GroundColor", &profileNight_.groundColor, { 0.2f, 0.2f, 0.2f });
        binder_->BindColor("Night_CloudAmbient", &profileNight_.cloudAmbientColor, { 0.8f, 0.8f, 0.8f });
        binder_->Bind("Night_AtmoGlow", &profileNight_.sunAtmosphereGlow, 0.5f, 0.01f, 0.0f, 2.0f);
        binder_->BindColor("Night_LightColor", &profileNight_.directionalLightColor, { 1.0f, 0.95f, 0.9f });
        binder_->Bind("Night_LightIntensity", &profileNight_.directionalLightIntensity, 1.0f, 0.01f, 0.0f, 5.0f);

        binder_->BindColor("Sunrise_ZenithColor", &profileSunrise_.zenithColor, { 0.05f, 0.15f, 0.4f });
        binder_->BindColor("Sunrise_HorizonColor", &profileSunrise_.horizonColor, { 0.4f, 0.6f, 0.8f });
        binder_->BindColor("Sunrise_GroundColor", &profileSunrise_.groundColor, { 0.2f, 0.2f, 0.2f });
        binder_->BindColor("Sunrise_CloudAmbient", &profileSunrise_.cloudAmbientColor, { 0.8f, 0.8f, 0.8f });
        binder_->Bind("Sunrise_AtmoGlow", &profileSunrise_.sunAtmosphereGlow, 0.5f, 0.01f, 0.0f, 2.0f);
        binder_->BindColor("Sunrise_LightColor", &profileSunrise_.directionalLightColor, { 1.0f, 0.95f, 0.9f });
        binder_->Bind("Sunrise_LightIntensity", &profileSunrise_.directionalLightIntensity, 1.0f, 0.01f, 0.0f, 5.0f);

        binder_->BindColor("Day_ZenithColor", &profileDay_.zenithColor, { 0.05f, 0.15f, 0.4f });
        binder_->BindColor("Day_HorizonColor", &profileDay_.horizonColor, { 0.4f, 0.6f, 0.8f });
        binder_->BindColor("Day_GroundColor", &profileDay_.groundColor, { 0.2f, 0.2f, 0.2f });
        binder_->BindColor("Day_CloudAmbient", &profileDay_.cloudAmbientColor, { 0.8f, 0.8f, 0.8f });
        binder_->Bind("Day_AtmoGlow", &profileDay_.sunAtmosphereGlow, 0.5f, 0.01f, 0.0f, 2.0f);
        binder_->BindColor("Day_LightColor", &profileDay_.directionalLightColor, { 1.0f, 0.95f, 0.9f });
        binder_->Bind("Day_LightIntensity", &profileDay_.directionalLightIntensity, 1.0f, 0.01f, 0.0f, 5.0f);

        binder_->BindColor("Sunset_ZenithColor", &profileSunset_.zenithColor, { 0.05f, 0.15f, 0.4f });
        binder_->BindColor("Sunset_HorizonColor", &profileSunset_.horizonColor, { 0.4f, 0.6f, 0.8f });
        binder_->BindColor("Sunset_GroundColor", &profileSunset_.groundColor, { 0.2f, 0.2f, 0.2f });
        binder_->BindColor("Sunset_CloudAmbient", &profileSunset_.cloudAmbientColor, { 0.8f, 0.8f, 0.8f });
        binder_->Bind("Sunset_AtmoGlow", &profileSunset_.sunAtmosphereGlow, 0.5f, 0.01f, 0.0f, 2.0f);
        binder_->BindColor("Sunset_LightColor", &profileSunset_.directionalLightColor, { 1.0f, 0.95f, 0.9f });
        binder_->Bind("Sunset_LightIntensity", &profileSunset_.directionalLightIntensity, 1.0f, 0.01f, 0.0f, 5.0f);

        // -------------------------------------------------------------
        // 天候プロファイル
        // -------------------------------------------------------------
        binder_->Bind("Sunny_TransitionSpeed", &profileSunny_.transitionSpeed, 0.1f, 0.005f, 0.001f, 2.0f);
        binder_->Bind("Sunny_CloudMin", &profileSunny_.cloudCoverageMin, 0.35f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Sunny_CloudMax", &profileSunny_.cloudCoverageMax, 0.70f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Sunny_CloudShadow", &profileSunny_.cloudShadowDensity, 0.60f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Sunny_LightDim", &profileSunny_.lightDimmer, 1.00f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Sunny_AtmoDim", &profileSunny_.atmosphereGlowDimmer, 1.00f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Sunny_Wetness", &profileSunny_.wetness, 0.00f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Sunny_RainIntens", &profileSunny_.rainIntensity, 0.0f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Sunny_WindDir", &profileSunny_.windDirection, { 1.0f, 0.0f }, 0.01f, -1.0f, 1.0f);
        binder_->Bind("Sunny_WindSpeed", &profileSunny_.windSpeed, 1.0f, 0.1f, 0.0f, 20.0f);
        binder_->Bind("Sunny_WindTurbul", &profileSunny_.windTurbulence, 0.2f, 0.01f, 0.0f, 1.0f);
        binder_->BindColor("Sunny_SkyZenith", &profileSunny_.skyZenithColor, { 0.05f, 0.15f, 0.4f });
        binder_->BindColor("Sunny_SkyHorizon", &profileSunny_.skyHorizonColor, { 0.4f, 0.6f, 0.8f });
        binder_->Bind("Sunny_SkyBlendWeight", &profileSunny_.skyColorBlendWeight, 0.00f, 0.01f, 0.0f, 1.0f);

        binder_->Bind("Cloudy_TransitionSpeed", &profileCloudy_.transitionSpeed, 0.1f, 0.005f, 0.001f, 2.0f);
        binder_->Bind("Cloudy_CloudMin", &profileCloudy_.cloudCoverageMin, 0.60f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Cloudy_CloudMax", &profileCloudy_.cloudCoverageMax, 1.00f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Cloudy_CloudShadow", &profileCloudy_.cloudShadowDensity, 0.80f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Cloudy_LightDim", &profileCloudy_.lightDimmer, 0.50f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Cloudy_AtmoDim", &profileCloudy_.atmosphereGlowDimmer, 0.50f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Cloudy_Wetness", &profileCloudy_.wetness, 0.00f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Cloudy_RainIntens", &profileCloudy_.rainIntensity, 0.0f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Cloudy_WindDir", &profileCloudy_.windDirection, { 1.0f, 0.0f }, 0.01f, -1.0f, 1.0f);
        binder_->Bind("Cloudy_WindSpeed", &profileCloudy_.windSpeed, 1.0f, 0.1f, 0.0f, 20.0f);
        binder_->Bind("Cloudy_WindTurbul", &profileCloudy_.windTurbulence, 0.2f, 0.01f, 0.0f, 1.0f);
        binder_->BindColor("Cloudy_SkyZenith", &profileCloudy_.skyZenithColor, { 0.2f, 0.25f, 0.3f });
        binder_->BindColor("Cloudy_SkyHorizon", &profileCloudy_.skyHorizonColor, { 0.4f, 0.42f, 0.45f });
        binder_->Bind("Cloudy_SkyBlendWeight", &profileCloudy_.skyColorBlendWeight, 0.40f, 0.01f, 0.0f, 1.0f);

        binder_->Bind("Rain_TransitionSpeed", &profileRain_.transitionSpeed, 0.1f, 0.005f, 0.001f, 2.0f);
        binder_->Bind("Rain_CloudMin", &profileRain_.cloudCoverageMin, 0.80f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_CloudMax", &profileRain_.cloudCoverageMax, 1.00f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_CloudShadow", &profileRain_.cloudShadowDensity, 0.90f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_LightDim", &profileRain_.lightDimmer, 0.20f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_AtmoDim", &profileRain_.atmosphereGlowDimmer, 0.20f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_Wetness", &profileRain_.wetness, 1.00f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_RainIntens", &profileRain_.rainIntensity, 0.8f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_WindDir", &profileRain_.windDirection, { 1.0f, 0.0f }, 0.01f, -1.0f, 1.0f);
        binder_->Bind("Rain_WindSpeed", &profileRain_.windSpeed, 3.0f, 0.1f, 0.0f, 20.0f);
        binder_->Bind("Rain_WindTurbul", &profileRain_.windTurbulence, 0.5f, 0.01f, 0.0f, 1.0f);
        binder_->BindColor("Rain_SkyZenith", &profileRain_.skyZenithColor, { 0.12f, 0.15f, 0.2f });
        binder_->BindColor("Rain_SkyHorizon", &profileRain_.skyHorizonColor, { 0.3f, 0.32f, 0.35f });
        binder_->Bind("Rain_SkyBlendWeight", &profileRain_.skyColorBlendWeight, 0.70f, 0.01f, 0.0f, 1.0f);

        binder_->Bind("Thunder_TransitionSpeed", &profileThunder_.transitionSpeed, 0.1f, 0.005f, 0.001f, 2.0f);
        binder_->Bind("Thunder_CloudMin", &profileThunder_.cloudCoverageMin, 0.80f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_CloudMax", &profileThunder_.cloudCoverageMax, 1.00f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_CloudShadow", &profileThunder_.cloudShadowDensity, 0.95f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_LightDim", &profileThunder_.lightDimmer, 0.15f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_AtmoDim", &profileThunder_.atmosphereGlowDimmer, 0.10f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_Wetness", &profileThunder_.wetness, 1.00f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_RainIntens", &profileThunder_.rainIntensity, 1.0f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_WindDir", &profileThunder_.windDirection, { 1.0f, 0.0f }, 0.01f, -1.0f, 1.0f);
        binder_->Bind("Thunder_WindSpeed", &profileThunder_.windSpeed, 5.0f, 0.1f, 0.0f, 20.0f);
        binder_->Bind("Thunder_WindTurbul", &profileThunder_.windTurbulence, 0.8f, 0.01f, 0.0f, 1.0f);
        binder_->BindColor("Thunder_SkyZenith", &profileThunder_.skyZenithColor, { 0.04f, 0.05f, 0.08f });
        binder_->BindColor("Thunder_SkyHorizon", &profileThunder_.skyHorizonColor, { 0.15f, 0.14f, 0.18f });
        binder_->Bind("Thunder_SkyBlendWeight", &profileThunder_.skyColorBlendWeight, 0.85f, 0.01f, 0.0f, 1.0f);
    }
}

void EnvironmentManager::Update(LightManager* lightManager)
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // 時間の進行と太陽・月のベクトル計算、基本時間帯プロファイルのブレンド
    timeOfDay_ += (deltaTime / 60.0f) * timeSpeedMultiplier_;
    while (timeOfDay_ >= 24.0f) { timeOfDay_ -= 24.0f; }
    while (timeOfDay_ < 0.0f) { timeOfDay_ += 24.0f; }

    float timeOffset = timeOfDay_ - 6.0f;
    float angle = (timeOffset / 24.0f) * (Math::PI * 2.0f);

    Vector3 sunDir;
    sunDir.x = -std::cosf(angle); sunDir.y = -std::sinf(angle); sunDir.z = 0.3f;
    float length = std::sqrtf(sunDir.x * sunDir.x + sunDir.y * sunDir.y + sunDir.z * sunDir.z);
    sunDir.x /= length; sunDir.y /= length; sunDir.z /= length;
    Vector3 moonDir = { -sunDir.x, -sunDir.y, -sunDir.z };
    sunDirection_ = sunDir;

    // 時間帯ブレンド (現在の timeOfDay_ に応じて currentProfile_ を計算)
    if (timeOfDay_ >= 5.0f && timeOfDay_ < 6.0f) {
        float t = (timeOfDay_ - 5.0f) / 1.0f;
        currentProfile_ = BlendProfiles(profileNight_, profileSunrise_, t);
    }
    else if (timeOfDay_ >= 6.0f && timeOfDay_ < 7.0f) {
        float t = (timeOfDay_ - 6.0f) / 1.0f;
        currentProfile_ = BlendProfiles(profileSunrise_, profileDay_, t);
    }
    else if (timeOfDay_ >= 7.0f && timeOfDay_ < 17.0f) {
        currentProfile_ = profileDay_;
    }
    else if (timeOfDay_ >= 17.0f && timeOfDay_ < 18.0f) {
        float t = (timeOfDay_ - 17.0f) / 1.0f;
        currentProfile_ = BlendProfiles(profileDay_, profileSunset_, t);
    }
    else if (timeOfDay_ >= 18.0f && timeOfDay_ < 19.0f) {
        float t = (timeOfDay_ - 18.0f) / 1.0f;
        currentProfile_ = BlendProfiles(profileSunset_, profileNight_, t);
    }
    else {
        currentProfile_ = profileNight_;
    }

    // 天候の遷移とPending状態の処理
    if (weatherTransitionT_ < 1.0f) {
        // 動的に計算された遷移スピードを適用
        float currentSpeed = GetCurrentTransitionSpeed();

        weatherTransitionT_ += deltaTime * currentSpeed;
        if (weatherTransitionT_ >= 1.0f) {
            weatherTransitionT_ = 1.0f;
            currentWeather_ = targetWeather_;

            if (hasPendingWeather_) {
                targetWeather_ = pendingWeather_;
                hasPendingWeather_ = false;
                weatherTransitionT_ = 0.0f;
            }
        }
    }

    WeatherProfile currentW = GetWeatherProfile(currentWeather_);
    WeatherProfile targetW = GetWeatherProfile(targetWeather_);

    // 天候パラメータのLerp
    currentWeatherProfile_.cloudCoverageMin = FE::Math::Lerp(currentW.cloudCoverageMin, targetW.cloudCoverageMin, weatherTransitionT_);
    currentWeatherProfile_.cloudCoverageMax = FE::Math::Lerp(currentW.cloudCoverageMax, targetW.cloudCoverageMax, weatherTransitionT_);
    currentWeatherProfile_.cloudShadowDensity = FE::Math::Lerp(currentW.cloudShadowDensity, targetW.cloudShadowDensity, weatherTransitionT_);
    currentWeatherProfile_.lightDimmer = FE::Math::Lerp(currentW.lightDimmer, targetW.lightDimmer, weatherTransitionT_);
    currentWeatherProfile_.atmosphereGlowDimmer = FE::Math::Lerp(currentW.atmosphereGlowDimmer, targetW.atmosphereGlowDimmer, weatherTransitionT_);
    currentWeatherProfile_.wetness = FE::Math::Lerp(currentW.wetness, targetW.wetness, weatherTransitionT_);
    currentWeatherProfile_.rainIntensity = FE::Math::Lerp(currentW.rainIntensity, targetW.rainIntensity, weatherTransitionT_);
    currentWeatherProfile_.windSpeed = FE::Math::Lerp(currentW.windSpeed, targetW.windSpeed, weatherTransitionT_);
    currentWeatherProfile_.windTurbulence = FE::Math::Lerp(currentW.windTurbulence, targetW.windTurbulence, weatherTransitionT_);
    currentWeatherProfile_.skyZenithColor = FE::Math::Lerp(currentW.skyZenithColor, targetW.skyZenithColor, weatherTransitionT_);
    currentWeatherProfile_.skyHorizonColor = FE::Math::Lerp(currentW.skyHorizonColor, targetW.skyHorizonColor, weatherTransitionT_);
    currentWeatherProfile_.skyColorBlendWeight = FE::Math::Lerp(currentW.skyColorBlendWeight, targetW.skyColorBlendWeight, weatherTransitionT_);

    // 現在の風向きと目標の風向きを角度(ラジアン)に変換
    float currentAngle = std::atan2(currentW.windDirection.y, currentW.windDirection.x);
    float targetAngle = std::atan2(targetW.windDirection.y, targetW.windDirection.x);

    // 角度の差分を計算（最短距離で回転させるための処理）
    float deltaAngle = targetAngle - currentAngle;
    while (deltaAngle > Math::PI)  deltaAngle -= Math::PI * 2.0f;
    while (deltaAngle < -Math::PI) deltaAngle += Math::PI * 2.0f;

    // 角度を線形補間
    float lerpedAngle = currentAngle + deltaAngle * weatherTransitionT_;

    // 補間された角度から新しい方向ベクトルを作成（すでに長さは1になります）
    currentWeatherProfile_.windDirection = { std::cos(lerpedAngle), std::sin(lerpedAngle) };

    // 時間帯 × 天候 の最終合成
    float finalLightIntensity = currentProfile_.directionalLightIntensity * currentWeatherProfile_.lightDimmer;

    auto lightData = lightManager->GetDirectionalLightData();

    if (sunDir.y <= 0.0f) {
        lightData[0].direction = sunDir;
    }
    else {
        lightData[0].direction = moonDir;
    }
    lightData[0].intensity = finalLightIntensity;
    lightData[0].color = { currentProfile_.directionalLightColor.x, currentProfile_.directionalLightColor.y, currentProfile_.directionalLightColor.z, 1.0f };

    // 天候による空の色の最終ブレンド合成処理
    float w = currentWeatherProfile_.skyColorBlendWeight;
    currentProfile_.zenithColor = FE::Math::Lerp(currentProfile_.zenithColor, currentWeatherProfile_.skyZenithColor, w);
    currentProfile_.horizonColor = FE::Math::Lerp(currentProfile_.horizonColor, currentWeatherProfile_.skyHorizonColor, w);

    // 大気散乱のGlow強さも天候によって減衰
    currentProfile_.sunAtmosphereGlow *= currentWeatherProfile_.atmosphereGlowDimmer;

    // 風専用の累積時間を更新 (deltaTime × 現在の補間済み風速)
    accumulatedWindTime_ += currentWeatherProfile_.windSpeed * deltaTime;
    windOffset_.x += currentWeatherProfile_.windDirection.x * currentWeatherProfile_.windSpeed * deltaTime;
    windOffset_.y += currentWeatherProfile_.windDirection.y * currentWeatherProfile_.windSpeed * deltaTime;

    if (cbData_)
    {
       /* cbData_->wetness = currentWeatherProfile_.wetness;*/
        cbData_->rainIntensity = currentWeatherProfile_.rainIntensity;
        cbData_->windDirection = currentWeatherProfile_.windDirection;
        cbData_->windSpeed = currentWeatherProfile_.windSpeed;
        cbData_->windTime = accumulatedWindTime_;
        cbData_->windOffset = windOffset_;
        cbData_->windTurbulence = currentWeatherProfile_.windTurbulence;
        cbData_->skyColor = { currentProfile_.zenithColor.x, currentProfile_.zenithColor.y, currentProfile_.zenithColor.z, 1.0f };
        cbData_->groundColor = { currentProfile_.groundColor.x, currentProfile_.groundColor.y, currentProfile_.groundColor.z, 1.0f };
    }
}

void EnvironmentManager::DebugDraw()
{
#ifdef ENABLE_IMGUI
    ImGui::Begin("環境設定");

    if (ImGui::CollapsingHeader("時間帯・ライティングプロファイル", ImGuiTreeNodeFlags_DefaultOpen))
    {
        binder_->Draw("TimeOfDay", "現在の時間 (時)");
        binder_->Draw("TimeSpeedMultiplier", "時間進行スピード");

        int hour = static_cast<int>(timeOfDay_);
        int minute = static_cast<int>((timeOfDay_ - hour) * 60.0f);
        ImGui::Text("Game Time: %02d:%02d", hour, minute);

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("TimeOfDayProfiles"))
        {
            if (ImGui::BeginTabItem("Night (0時/24時)")) {
                binder_->Draw("Night_ZenithColor", "天頂の色");
                binder_->Draw("Night_HorizonColor", "地平線の色");
                binder_->Draw("Night_GroundColor", "地面の色");
                binder_->Draw("Night_CloudAmbient", "雲の環境光");
                binder_->Draw("Night_AtmoGlow", "大気散乱の強さ");
                binder_->Draw("Night_LightColor", "環境光/月明かりの色");
                binder_->Draw("Night_LightIntensity", "光の強さ");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Sunrise (6時)")) {
                binder_->Draw("Sunrise_ZenithColor", "天頂の色");
                binder_->Draw("Sunrise_HorizonColor", "地平線の色");
                binder_->Draw("Sunrise_GroundColor", "地面の色");
                binder_->Draw("Sunrise_CloudAmbient", "雲の環境光");
                binder_->Draw("Sunrise_AtmoGlow", "大気散乱の強さ");
                binder_->Draw("Sunrise_LightColor", "太陽光の色");
                binder_->Draw("Sunrise_LightIntensity", "光の強さ");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Day (12時)")) {
                binder_->Draw("Day_ZenithColor", "天頂の色");
                binder_->Draw("Day_HorizonColor", "地平線の色");
                binder_->Draw("Day_GroundColor", "地面の色");
                binder_->Draw("Day_CloudAmbient", "雲の環境光");
                binder_->Draw("Day_AtmoGlow", "大気散乱の強さ");
                binder_->Draw("Day_LightColor", "太陽光の色");
                binder_->Draw("Day_LightIntensity", "光の強さ");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Sunset (18時)")) {
                binder_->Draw("Sunset_ZenithColor", "天頂の色");
                binder_->Draw("Sunset_HorizonColor", "地平線の色");
                binder_->Draw("Sunset_GroundColor", "地面の色");
                binder_->Draw("Sunset_CloudAmbient", "雲の環境光");
                binder_->Draw("Sunset_AtmoGlow", "大気散乱の強さ");
                binder_->Draw("Sunset_LightColor", "太陽光の色");
                binder_->Draw("Sunset_LightIntensity", "光の強さ");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    if (ImGui::CollapsingHeader("天候・ウェザープロファイル", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const char* weatherNames[] = { "Sunny", "Cloudy", "Rain", "Thunderstorm" };
        ImGui::Text("Current Weather: %s", weatherNames[static_cast<int>(currentWeather_)]);
        ImGui::Text("Target Weather: %s", weatherNames[static_cast<int>(targetWeather_)]);

        ImGui::ProgressBar(weatherTransitionT_, ImVec2(0.0f, 0.0f), "Transition Progress");
        if (hasPendingWeather_) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Pending: %s", weatherNames[static_cast<int>(pendingWeather_)]);
        }

        ImGui::Spacing();
        ImGui::Text("天候を変更する (テスト用)");

        if (ImGui::Button("Sunny")) RequestWeatherChange(WeatherState::Sunny); ImGui::SameLine();
        if (ImGui::Button("Cloudy")) RequestWeatherChange(WeatherState::Cloudy); ImGui::SameLine();
        if (ImGui::Button("Rain")) RequestWeatherChange(WeatherState::Rain); ImGui::SameLine();
        if (ImGui::Button("Thunder")) RequestWeatherChange(WeatherState::Thunderstorm);

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("WeatherProfiles"))
        {
            const char* prefix[] = { "Sunny", "Cloudy", "Rain", "Thunder" };
            const char* tabNames[] = { "Sunny", "Cloudy", "Rain", "Thunderstorm" };

            for (int i = 0; i < 4; ++i) {
                if (ImGui::BeginTabItem(tabNames[i])) {
                    std::string p = prefix[i];
                    binder_->Draw((p + "_TransitionSpeed").c_str(), "この天候のベース遷移スピード");
                    ImGui::Separator();
                    binder_->Draw((p + "_CloudMin").c_str(), "雲の量 (下限)");
                    binder_->Draw((p + "_CloudMax").c_str(), "雲の量 (上限)");
                    binder_->Draw((p + "_CloudShadow").c_str(), "雲の影の濃さ");
                    binder_->Draw((p + "_LightDim").c_str(), "太陽/月の光の強さ倍率");
                    binder_->Draw((p + "_AtmoDim").c_str(), "大気散乱の強さ倍率");
                    binder_->Draw((p + "_Wetness").c_str(), "地面の濡れ具合");

                    ImGui::Separator();
                    ImGui::Text("雨と風の環境設定");
                    binder_->Draw((p + "_RainIntens").c_str(), "雨(雪)の強さ");
                    binder_->Draw((p + "_WindDir").c_str(), "風向き");
                    binder_->Draw((p + "_WindSpeed").c_str(), "風速");
                    binder_->Draw((p + "_WindTurbul").c_str(), "風の乱れ");

                    ImGui::Separator();
                    ImGui::Text("天候固有の空の色設定");
                    binder_->Draw((p + "_SkyZenith").c_str(), "天候時の天頂の色");
                    binder_->Draw((p + "_SkyHorizon").c_str(), "天候時の地平線の色");
                    binder_->Draw((p + "_SkyBlendWeight").c_str(), "天候カラーへの強制ブレンド率");

                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }

    ImGui::End();
#endif
}

void EnvironmentManager::Finalize()
{
    // ComPtr をすべて明示的に解放する
    constantBuffer_.Reset();
}

void EnvironmentManager::RequestWeatherChange(WeatherState nextWeather)
{
    // 晴れから直接雨・雪・雷に行こうとした場合
    if (currentWeather_ == WeatherState::Sunny &&
        (nextWeather == WeatherState::Rain || nextWeather == WeatherState::Thunderstorm))
    {
        targetWeather_ = WeatherState::Cloudy; // 一旦ターゲットを曇りにする
        pendingWeather_ = nextWeather;         // 天候を覚えておく
        hasPendingWeather_ = true;
    }
    else
    {
        targetWeather_ = nextWeather;
        hasPendingWeather_ = false;
    }
    weatherTransitionT_ = 0.0f; // 遷移開始
}

WeatherProfile EnvironmentManager::GetWeatherProfile(WeatherState state) const
{
    switch (state) {
    case WeatherState::Sunny:        return profileSunny_;
    case WeatherState::Cloudy:       return profileCloudy_;
    case WeatherState::Rain:         return profileRain_;
    case WeatherState::Thunderstorm: return profileThunder_;
    default:                         return profileSunny_;
    }
}

float EnvironmentManager::GetCurrentTransitionSpeed() const
{
    // 遷移先の天候プロファイルを取得
    WeatherProfile targetW = GetWeatherProfile(targetWeather_);

    // 遷移先の天候のスピードを返す
    return targetW.transitionSpeed;
}

}