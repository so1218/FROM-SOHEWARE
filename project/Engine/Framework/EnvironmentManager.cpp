#include "pch.h"
#include "EnvironmentManager.h"
#include "TimeManager.h"
#include "LightManager.h"
#include "Engine.h"
#include "GraphicsDevice.h"
#include "PropertyBinder.h"
#include "EnvironmentWindow.h"

namespace FS
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
        binder_->Bind("Night_AtmoGlow", &profileNight_.sunAtmosphereGlow, 0.5f, 0.01f, 0.0f, 2.0f);
        binder_->BindColor("Night_LightColor", &profileNight_.directionalLightColor, { 1.0f, 0.95f, 0.9f });
        binder_->Bind("Night_LightIntensity", &profileNight_.directionalLightIntensity, 1.0f, 0.01f, 0.0f, 5.0f);
        binder_->BindColor("Night_AmbientSky", &profileNight_.ambientSkyColor, { 0.015f, 0.02f, 0.04f });
        binder_->BindColor("Night_AmbientGround", &profileNight_.ambientGroundColor, { 0.005f, 0.005f, 0.008f });

        binder_->BindColor("Sunrise_ZenithColor", &profileSunrise_.zenithColor, { 0.05f, 0.15f, 0.4f });
        binder_->BindColor("Sunrise_HorizonColor", &profileSunrise_.horizonColor, { 0.4f, 0.6f, 0.8f });
        binder_->Bind("Sunrise_AtmoGlow", &profileSunrise_.sunAtmosphereGlow, 0.5f, 0.01f, 0.0f, 2.0f);
        binder_->BindColor("Sunrise_LightColor", &profileSunrise_.directionalLightColor, { 1.0f, 0.95f, 0.9f });
        binder_->Bind("Sunrise_LightIntensity", &profileSunrise_.directionalLightIntensity, 1.0f, 0.01f, 0.0f, 5.0f);
        binder_->BindColor("Sunrise_AmbientSky", &profileSunrise_.ambientSkyColor, { 0.2f, 0.16f, 0.15f });
        binder_->BindColor("Sunrise_AmbientGround", &profileSunrise_.ambientGroundColor, { 0.06f, 0.05f, 0.04f });

        binder_->BindColor("Day_ZenithColor", &profileDay_.zenithColor, { 0.05f, 0.15f, 0.4f });
        binder_->BindColor("Day_HorizonColor", &profileDay_.horizonColor, { 0.4f, 0.6f, 0.8f });
        binder_->Bind("Day_AtmoGlow", &profileDay_.sunAtmosphereGlow, 0.5f, 0.01f, 0.0f, 2.0f);
        binder_->BindColor("Day_LightColor", &profileDay_.directionalLightColor, { 1.0f, 0.95f, 0.9f });
        binder_->Bind("Day_LightIntensity", &profileDay_.directionalLightIntensity, 1.0f, 0.01f, 0.0f, 5.0f);
        binder_->BindColor("Day_AmbientSky", &profileDay_.ambientSkyColor, { 0.25f, 0.28f, 0.33f });
        binder_->BindColor("Day_AmbientGround", &profileDay_.ambientGroundColor, { 0.08f, 0.07f, 0.05f });

        binder_->BindColor("Sunset_ZenithColor", &profileSunset_.zenithColor, { 0.05f, 0.15f, 0.4f });
        binder_->BindColor("Sunset_HorizonColor", &profileSunset_.horizonColor, { 0.4f, 0.6f, 0.8f });
        binder_->Bind("Sunset_AtmoGlow", &profileSunset_.sunAtmosphereGlow, 0.5f, 0.01f, 0.0f, 2.0f);
        binder_->BindColor("Sunset_LightColor", &profileSunset_.directionalLightColor, { 1.0f, 0.95f, 0.9f });
        binder_->Bind("Sunset_LightIntensity", &profileSunset_.directionalLightIntensity, 1.0f, 0.01f, 0.0f, 5.0f);
        binder_->BindColor("Sunset_AmbientSky", &profileSunset_.ambientSkyColor, { 0.2f, 0.14f, 0.13f });
        binder_->BindColor("Sunset_AmbientGround", &profileSunset_.ambientGroundColor, { 0.06f, 0.04f, 0.03f });

        // -------------------------------------------------------------
        // 天候プロファイル
        // -------------------------------------------------------------
        binder_->Bind("Sunny_TransitionSpeed", &profileSunny_.transitionSpeed, 0.1f, 0.005f, 0.001f, 2.0f);
        binder_->Bind("Sunny_CloudMin", &profileSunny_.cloudThreshold, 0.35f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Sunny_CloudShadow", &profileSunny_.cloudShadowBrightness, 0.60f, 0.01f, 0.0f, 1.0f);
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

        binder_->Bind("Rain_TransitionSpeed", &profileRain_.transitionSpeed, 0.1f, 0.005f, 0.001f, 2.0f);
        binder_->Bind("Rain_CloudMin", &profileRain_.cloudThreshold, 0.80f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_CloudShadow", &profileRain_.cloudShadowBrightness, 0.90f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_LightDim", &profileRain_.lightDimmer, 0.20f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_AtmoDim", &profileRain_.atmosphereGlowDimmer, 0.20f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_Wetness", &profileRain_.wetness, 0.50f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_RainIntens", &profileRain_.rainIntensity, 0.8f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Rain_WindDir", &profileRain_.windDirection, { 1.0f, 0.0f }, 0.01f, -1.0f, 1.0f);
        binder_->Bind("Rain_WindSpeed", &profileRain_.windSpeed, 3.0f, 0.1f, 0.0f, 20.0f);
        binder_->Bind("Rain_WindTurbul", &profileRain_.windTurbulence, 0.5f, 0.01f, 0.0f, 1.0f);
        binder_->BindColor("Rain_SkyZenith", &profileRain_.skyZenithColor, { 0.12f, 0.15f, 0.2f });
        binder_->BindColor("Rain_SkyHorizon", &profileRain_.skyHorizonColor, { 0.3f, 0.32f, 0.35f });
        binder_->Bind("Rain_SkyBlendWeight", &profileRain_.skyColorBlendWeight, 0.70f, 0.01f, 0.0f, 1.0f);

        binder_->Bind("Thunder_TransitionSpeed", &profileThunder_.transitionSpeed, 0.1f, 0.005f, 0.001f, 2.0f);
        binder_->Bind("Thunder_CloudMin", &profileThunder_.cloudThreshold, 0.80f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_CloudShadow", &profileThunder_.cloudShadowBrightness, 0.95f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_LightDim", &profileThunder_.lightDimmer, 0.15f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_AtmoDim", &profileThunder_.atmosphereGlowDimmer, 0.10f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_Wetness", &profileThunder_.wetness, 0.50f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_RainIntens", &profileThunder_.rainIntensity, 1.0f, 0.01f, 0.0f, 1.0f);
        binder_->Bind("Thunder_WindDir", &profileThunder_.windDirection, { 1.0f, 0.0f }, 0.01f, -1.0f, 1.0f);
        binder_->Bind("Thunder_WindSpeed", &profileThunder_.windSpeed, 5.0f, 0.1f, 0.0f, 20.0f);
        binder_->Bind("Thunder_WindTurbul", &profileThunder_.windTurbulence, 0.8f, 0.01f, 0.0f, 1.0f);
        binder_->BindColor("Thunder_SkyZenith", &profileThunder_.skyZenithColor, { 0.04f, 0.05f, 0.08f });
        binder_->BindColor("Thunder_SkyHorizon", &profileThunder_.skyHorizonColor, { 0.15f, 0.14f, 0.18f });
        binder_->Bind("Thunder_SkyBlendWeight", &profileThunder_.skyColorBlendWeight, 0.85f, 0.01f, 0.0f, 1.0f);

        auto bindVisual = [&](const std::string& p, WeatherVisualParams& v)
            {
                // Terrain
                binder_->Bind((p + "_V_Metal").c_str(), &v.metalness, v.metalness, 0.01f, 0.0f, 1.0f);
                binder_->Bind((p + "_V_Rough").c_str(), &v.roughness, v.roughness, 0.01f, 0.0f, 1.0f);
                binder_->Bind((p + "_V_EnvIntens").c_str(), &v.environmentMapIntensity, v.environmentMapIntensity, 0.01f, 0.0f, 5.0f);
                binder_->Bind((p + "_V_RippleSize").c_str(), &v.rippleSize, v.rippleSize, 0.1f, 0.0f, 10.0f);
                binder_->Bind((p + "_V_NormalIntens").c_str(), &v.normalIntensity, v.normalIntensity, 0.05f, 0.0f, 5.0f);
                binder_->BindColor((p + "_V_Color").c_str(), &v.color, v.color);
                binder_->Bind((p + "_V_Emissive").c_str(), &v.emissiveIntensity, v.emissiveIntensity, 0.1f, 0.0f, 50.0f);

                // Volumetric Fog
                binder_->Bind((p + "_V_FogNoiseScale").c_str(), &v.noiseScale, v.noiseScale, 0.001f, 0.001f, 1.0f);
                binder_->Bind((p + "_V_FogNoiseIntens").c_str(), &v.noiseIntensity, v.noiseIntensity, 0.05f, 0.0f, 5.0f);
                binder_->Bind((p + "_V_FogHDensity").c_str(), &v.heightDensity, v.heightDensity, 0.05f, 0.0f, 10.0f);
                binder_->Bind((p + "_V_FogHFalloff").c_str(), &v.heightFalloff, v.heightFalloff, 0.01f, 0.0f, 2.0f);
                binder_->BindColor((p + "_V_FogAmbient").c_str(), &v.ambientLight, v.ambientLight);
                binder_->Bind((p + "_V_FogExtinct").c_str(), &v.extinction, v.extinction, 0.001f, 0.0f, 1.0f);
                binder_->Bind((p + "_V_FogErosion").c_str(), &v.erosion, v.erosion, 0.01f, 0.0f, 1.0f);
                binder_->Bind((p + "_V_FogWindSpeedMultiplier").c_str(), &v.windSpeedMultiplier, v.windSpeedMultiplier, 0.05f, 0.0f, 10.0f);
            };

        bindVisual("Sunny", profileSunny_.visual);
        bindVisual("Rain", profileRain_.visual);
        bindVisual("Thunder", profileThunder_.visual);
    }
}

inline WeatherVisualParams LerpVisualParams(const WeatherVisualParams& a, const WeatherVisualParams& b, float t)
{
    WeatherVisualParams res;
    res.metalness = FS::Math::Lerp(a.metalness, b.metalness, t);
    res.roughness = FS::Math::Lerp(a.roughness, b.roughness, t);
    res.environmentMapIntensity = FS::Math::Lerp(a.environmentMapIntensity, b.environmentMapIntensity, t);
    res.rippleSize = FS::Math::Lerp(a.rippleSize, b.rippleSize, t);
    res.normalIntensity = FS::Math::Lerp(a.normalIntensity, b.normalIntensity, t);
    res.color = {
        FS::Math::Lerp(a.color.x, b.color.x, t),
        FS::Math::Lerp(a.color.y, b.color.y, t),
        FS::Math::Lerp(a.color.z, b.color.z, t),
        FS::Math::Lerp(a.color.w, b.color.w, t)
    };
    res.emissiveIntensity = FS::Math::Lerp(a.emissiveIntensity, b.emissiveIntensity, t);

    res.noiseScale = FS::Math::Lerp(a.noiseScale, b.noiseScale, t);
    res.noiseIntensity = FS::Math::Lerp(a.noiseIntensity, b.noiseIntensity, t);
    res.heightDensity = FS::Math::Lerp(a.heightDensity, b.heightDensity, t);
    res.heightFalloff = FS::Math::Lerp(a.heightFalloff, b.heightFalloff, t);
    res.ambientLight = {
        FS::Math::Lerp(a.ambientLight.x, b.ambientLight.x, t),
        FS::Math::Lerp(a.ambientLight.y, b.ambientLight.y, t),
        FS::Math::Lerp(a.ambientLight.z, b.ambientLight.z, t)
    };
    res.extinction = FS::Math::Lerp(a.extinction, b.extinction, t);
    res.erosion = FS::Math::Lerp(a.erosion, b.erosion, t);
    res.windSpeedMultiplier = FS::Math::Lerp(a.windSpeedMultiplier, b.windSpeedMultiplier, t);
    return res;
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

    // 天候の遷移
    if (weatherTransitionT_ < 1.0f) {
        // 動的に計算された遷移スピードを適用
        float currentSpeed = GetCurrentTransitionSpeed();

        weatherTransitionT_ += deltaTime * currentSpeed;
        if (weatherTransitionT_ >= 1.0f)
        {
            weatherTransitionT_ = 1.0f;
            currentWeather_ = targetWeather_;
        }
    }

    WeatherProfile currentW = GetWeatherProfile(currentWeather_);
    WeatherProfile targetW = GetWeatherProfile(targetWeather_);

    // 天候パラメータのLerp
    currentWeatherProfile_.cloudThreshold = FS::Math::Lerp(currentW.cloudThreshold, targetW.cloudThreshold, weatherTransitionT_);
    currentWeatherProfile_.cloudShadowBrightness = FS::Math::Lerp(currentW.cloudShadowBrightness, targetW.cloudShadowBrightness, weatherTransitionT_);
    currentWeatherProfile_.lightDimmer = FS::Math::Lerp(currentW.lightDimmer, targetW.lightDimmer, weatherTransitionT_);
    currentWeatherProfile_.atmosphereGlowDimmer = FS::Math::Lerp(currentW.atmosphereGlowDimmer, targetW.atmosphereGlowDimmer, weatherTransitionT_);
    currentWeatherProfile_.wetness = FS::Math::Lerp(currentW.wetness, targetW.wetness, weatherTransitionT_);
    currentWeatherProfile_.rainIntensity = FS::Math::Lerp(currentW.rainIntensity, targetW.rainIntensity, weatherTransitionT_);
    currentWeatherProfile_.windSpeed = FS::Math::Lerp(currentW.windSpeed, targetW.windSpeed, weatherTransitionT_);
    currentWeatherProfile_.windTurbulence = FS::Math::Lerp(currentW.windTurbulence, targetW.windTurbulence, weatherTransitionT_);
    currentWeatherProfile_.skyZenithColor = FS::Math::Lerp(currentW.skyZenithColor, targetW.skyZenithColor, weatherTransitionT_);
    currentWeatherProfile_.skyHorizonColor = FS::Math::Lerp(currentW.skyHorizonColor, targetW.skyHorizonColor, weatherTransitionT_);
    currentWeatherProfile_.skyColorBlendWeight = FS::Math::Lerp(currentW.skyColorBlendWeight, targetW.skyColorBlendWeight, weatherTransitionT_);

    currentVisualParams_ = LerpVisualParams(currentW.visual, targetW.visual, weatherTransitionT_);

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
    currentProfile_.zenithColor = FS::Math::Lerp(currentProfile_.zenithColor, currentWeatherProfile_.skyZenithColor, w);
    currentProfile_.horizonColor = FS::Math::Lerp(currentProfile_.horizonColor, currentWeatherProfile_.skyHorizonColor, w);

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

        // 雨や雷雨では空の光が雲で散らされるので、天候が空の色を変える強さだけ環境光の色味を抜く
        auto removeTint = [w](const Vector3& color)
            {
                float luminance = color.x * 0.2126f + color.y * 0.7152f + color.z * 0.0722f;
                return FS::Math::Lerp(color, Vector3{ luminance, luminance, luminance }, w);
            };
        const Vector3 ambientSky = removeTint(currentProfile_.ambientSkyColor);
        const Vector3 ambientGround = removeTint(currentProfile_.ambientGroundColor);
        cbData_->ambientSkyColor = { ambientSky.x, ambientSky.y, ambientSky.z, 1.0f };
        cbData_->ambientGroundColor = { ambientGround.x, ambientGround.y, ambientGround.z, 1.0f };
    }
}

void EnvironmentManager::DebugGui()
{
    if (BeginEnvironmentTab("時間帯"))
    {
        int hour = static_cast<int>(timeOfDay_);
        int minute = static_cast<int>((timeOfDay_ - hour) * 60.0f);
        ImGui::Text("現在のゲーム時間: %02d:%02d", hour, minute);

        binder_->Draw("TimeOfDay", "時間 (0-24時)");
        binder_->Draw("TimeSpeedMultiplier", "進行スピード倍率");

        ImGui::Spacing();
        if (ImGui::BeginTabBar("TimeOfDayProfiles"))
        {
            auto drawTimeTab = [&](const char* label, const char* p) {
                if (ImGui::BeginTabItem(label)) {
                    binder_->Draw((std::string(p) + "_ZenithColor").c_str(), "天頂の色");
                    binder_->Draw((std::string(p) + "_HorizonColor").c_str(), "地平線の色");
                    binder_->Draw((std::string(p) + "_AtmoGlow").c_str(), "大気散乱強度");
                    binder_->Draw((std::string(p) + "_LightColor").c_str(), "太陽/月光色");
                    binder_->Draw((std::string(p) + "_LightIntensity").c_str(), "光強度");
                    binder_->Draw((std::string(p) + "_AmbientSky").c_str(), "環境光 (上からの光)");
                    binder_->Draw((std::string(p) + "_AmbientGround").c_str(), "環境光 (地面からの照り返し)");
                    ImGui::EndTabItem();
                }
                };

            drawTimeTab("Night (0時)", "Night");
            drawTimeTab("Sunrise (6時)", "Sunrise");
            drawTimeTab("Day (12時)", "Day");
            drawTimeTab("Sunset (18時)", "Sunset");

            ImGui::EndTabBar();
        }
        EndEnvironmentTab();
    }

    if (BeginEnvironmentTab("天候"))
    {
        const char* weatherNames[] = { "Sunny (晴れ)", "Rain (雨)", "Thunderstorm (雷雨)" };

        ImGui::Text("現在: %s  ->  目標: %s",
            weatherNames[static_cast<int>(currentWeather_)],
            weatherNames[static_cast<int>(targetWeather_)]);

        // 目標の天候のボタンを色付けする
        auto drawWeatherButton = [&](const char* label, WeatherState state) {
            bool isActive = (targetWeather_ == state);
            if (isActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
            }
            if (ImGui::Button(label, ImVec2(80.0f, 0.0f))) {
                RequestWeatherChange(state);
            }
            if (isActive) {
                ImGui::PopStyleColor();
            }
            };

        drawWeatherButton("晴れ", WeatherState::Sunny); ImGui::SameLine();
        drawWeatherButton("雨", WeatherState::Rain); ImGui::SameLine();
        drawWeatherButton("雷雨", WeatherState::Thunderstorm);

        ImGui::SeparatorText("天候ごとの設定");

        int editingIndex = static_cast<int>(editingWeather_);
        if (ImGui::Combo("編集する天候", &editingIndex, weatherNames, IM_ARRAYSIZE(weatherNames)))
        {
            editingWeather_ = static_cast<WeatherState>(editingIndex);
        }

        const char* prefixes[] = { "Sunny", "Rain", "Thunder" };
        std::string p = prefixes[editingIndex];

        binder_->Draw((p + "_TransitionSpeed").c_str(), "天候遷移スピード");

        if (ImGui::TreeNode("雲 & 大気設定"))
        {
            binder_->Draw((p + "_CloudMin").c_str(), "雲が出る濃さ (大きいほど雲が減る)");
            binder_->Draw((p + "_CloudShadow").c_str(), "雲の陰の明るさ");
            binder_->Draw((p + "_LightDim").c_str(), "太陽/月光の遮蔽率");
            binder_->Draw((p + "_AtmoDim").c_str(), "大気散乱の減衰率");
            binder_->Draw((p + "_Wetness").c_str(), "地面の濡れ率");
            binder_->Draw((p + "_SkyZenith").c_str(), "天頂の色");
            binder_->Draw((p + "_SkyHorizon").c_str(), "地平線の色");
            binder_->Draw((p + "_SkyBlendWeight").c_str(), "天候カラー強制補間率");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("雨 & 風設定"))
        {
            binder_->Draw((p + "_RainIntens").c_str(), "雨パーティクル強度");
            binder_->Draw((p + "_WindDir").c_str(), "風向き");
            binder_->Draw((p + "_WindSpeed").c_str(), "風速");
            binder_->Draw((p + "_WindTurbul").c_str(), "風の乱れ");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Terrain (地形質感) 設定"))
        {
            binder_->Draw((p + "_V_Metal").c_str(), "Metalness");
            binder_->Draw((p + "_V_Rough").c_str(), "Roughness");
            binder_->Draw((p + "_V_EnvIntens").c_str(), "環境マップ強度");
            binder_->Draw((p + "_V_RippleSize").c_str(), "波紋サイズ");
            binder_->Draw((p + "_V_NormalIntens").c_str(), "法線強度");
            binder_->Draw((p + "_V_Color").c_str(), "ベースカラー");
            binder_->Draw((p + "_V_Emissive").c_str(), "発光強度");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Volumetric Fog (フォグ) 設定"))
        {
            binder_->Draw((p + "_V_FogNoiseScale").c_str(), "ノイズスケール");
            binder_->Draw((p + "_V_FogNoiseIntens").c_str(), "ノイズ強度");
            binder_->Draw((p + "_V_FogHDensity").c_str(), "高さ密度");
            binder_->Draw((p + "_V_FogHFalloff").c_str(), "高さ減衰");
            binder_->Draw((p + "_V_FogAmbient").c_str(), "環境光");
            binder_->Draw((p + "_V_FogExtinct").c_str(), "消去係数 (Extinction)");
            binder_->Draw((p + "_V_FogErosion").c_str(), "削り取り (Erosion)");
            binder_->Draw((p + "_V_FogWindSpeedMultiplier").c_str(), "フォグ風速倍率");
            ImGui::TreePop();
        }

        EndEnvironmentTab();
    }
}

void EnvironmentManager::Finalize()
{
    // ComPtr をすべて明示的に解放する
    constantBuffer_.Reset();
}

void EnvironmentManager::RequestWeatherChange(WeatherState nextWeather)
{
    targetWeather_ = nextWeather;
    weatherTransitionT_ = 0.0f; // 遷移開始
}

WeatherProfile EnvironmentManager::GetWeatherProfile(WeatherState state) const
{
    switch (state) {
    case WeatherState::Sunny:        return profileSunny_;
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