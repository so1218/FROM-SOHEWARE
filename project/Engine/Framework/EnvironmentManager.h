#pragma once
#include "MathUtils.h"
#include "Structures.h"

namespace FE
{

class LightManager;
class Engine;
class PropertyBinder;

struct TimeOfDayProfile
{
    // 空の色
    Vector3 zenithColor;
    Vector3 horizonColor;
    Vector3 groundColor;
    // 雲と大気
    Vector3 cloudAmbientColor;
    float sunAtmosphereGlow;
    // ライト（ディレクショナルライト）
    Vector3 directionalLightColor;
    float directionalLightIntensity;
};

// 天候の種類
enum class WeatherState {
    Sunny,          
    Cloudy,       
    Rain,             
    Thunderstorm    
};

// 天候が環境に与える影響のプロファイル
struct WeatherProfile
{
    float cloudCoverageMin;      // 雲の量（下限）
    float cloudCoverageMax;      // 雲の量（上限）
    float cloudShadowDensity;    // 雲の影の濃さ
    float lightDimmer;           // 太陽/月の光をどれくらい遮るか (1.0 = そのまま, 0.2 = 暗い)
    float atmosphereGlowDimmer;  // 大気散乱の減衰
    float wetness;
    float rainIntensity;
    Vector2 windDirection;
    float windSpeed;
    float windTurbulence;
    Vector3 skyZenithColor;      // この天候のときの天頂の色
    Vector3 skyHorizonColor;     // この天候のときの地平線の色
    float skyColorBlendWeight;
    float transitionSpeed;
};

// プロファイル同士をブレンドする関数
inline TimeOfDayProfile BlendProfiles(const TimeOfDayProfile& a, const TimeOfDayProfile& b, float t)
{
    TimeOfDayProfile result;
    result.zenithColor = FE::Math::Lerp(a.zenithColor, b.zenithColor, t);
    result.horizonColor = FE::Math::Lerp(a.horizonColor, b.horizonColor, t);
    result.groundColor = FE::Math::Lerp(a.groundColor, b.groundColor, t);
    result.cloudAmbientColor = FE::Math::Lerp(a.cloudAmbientColor, b.cloudAmbientColor, t);
    result.sunAtmosphereGlow = FE::Math::Lerp(a.sunAtmosphereGlow, b.sunAtmosphereGlow, t);
    result.directionalLightColor = FE::Math::Lerp(a.directionalLightColor, b.directionalLightColor, t);
    result.directionalLightIntensity = FE::Math::Lerp(a.directionalLightIntensity, b.directionalLightIntensity, t);
    return result;
}

class EnvironmentManager
{
public:
    static EnvironmentManager* GetInstance() {
        static EnvironmentManager instance;
        return &instance;
    }

    // 初期化関数
    void Initialize(Engine* engine);

    void Update(LightManager* lightManager);
    void DebugDraw();

    void Finalize();

    float GetTimeOfDay() const { return timeOfDay_; }
    void SetTimeOfDay(float time) { timeOfDay_ = time; }
    Vector3 GetSunDirection() const { return sunDirection_; }

    const TimeOfDayProfile& GetCurrentProfile() const { return currentProfile_; }

    // Skydome側から現在の天候プロファイルを取得するための関数
    const WeatherProfile& GetCurrentWeatherProfile() const { return currentWeatherProfile_; }

    WeatherState GetCurrentWeather() const { return currentWeather_; }
    WeatherState GetTargetWeather() const { return targetWeather_; }
    float GetWeatherTransitionProgress() const { return weatherTransitionT_; }

    // 現在の遷移状態に応じたスピードを返す
    float GetCurrentTransitionSpeed() const;

    ID3D12Resource* GetGlobalEnvironmentResource() const { return constantBuffer_.Get(); }

    // 現在の風のパラメータを取得するゲッター
    Vector2 GetWindDirection() const { return currentWeatherProfile_.windDirection; }
    float GetWindSpeed() const { return currentWeatherProfile_.windSpeed; }

private:
    EnvironmentManager();
    ~EnvironmentManager();

    // コピー禁止
    EnvironmentManager(const EnvironmentManager&) = delete;
    EnvironmentManager& operator=(const EnvironmentManager&) = delete;

    void RequestWeatherChange(WeatherState nextWeather);
    // 天候のEnumから対応するプロファイルを返すヘルパー関数
    WeatherProfile GetWeatherProfile(WeatherState state) const;

    std::unique_ptr<PropertyBinder> binder_;

    // パラメータ
    float timeOfDay_ = 12.0f;
    float timeSpeedMultiplier_ = 24.0f;
    Vector3 sunDirection_ = { 0.0f, -1.0f, 0.0f };

    TimeOfDayProfile profileNight_;   // 0時
    TimeOfDayProfile profileSunrise_; // 6時
    TimeOfDayProfile profileDay_;     // 12時
    TimeOfDayProfile profileSunset_;  // 18時

    // ブレンドされた現在の結果を保存する変数
    TimeOfDayProfile currentProfile_;

    WeatherState currentWeather_ = WeatherState::Sunny;
    WeatherState targetWeather_ = WeatherState::Sunny;
    float weatherTransitionT_ = 1.0f; 
    float accumulatedWindTime_ = 0.0f;
    Vector2 windOffset_ = { 0.0f, 0.0f };

    WeatherProfile profileSunny_, profileCloudy_, profileRain_, profileThunder_;
    WeatherProfile currentWeatherProfile_;

    WeatherState pendingWeather_ = WeatherState::Sunny;
    bool hasPendingWeather_ = false;

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    GlobalEnvironmentData* cbData_ = nullptr;
};

}