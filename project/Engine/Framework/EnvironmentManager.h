#pragma once
#include "MathUtils.h"
#include "Structures.h"

namespace FS
{

class LightManager;
class Engine;
class PropertyBinder;

struct TimeOfDayProfile
{
    // 空の色
    Vector3 zenithColor;
    Vector3 horizonColor;
    // 雲と大気
    float sunAtmosphereGlow;
    // ライト
    Vector3 directionalLightColor;
    float directionalLightIntensity;
    // 環境光。空の見た目の色とは別に決める
    Vector3 ambientSkyColor;
    Vector3 ambientGroundColor;
};

// 天候の種類
enum class WeatherState 
{
    Sunny,          
    Rain,             
    Thunderstorm    
};

// Volumetric Fog の見た目
struct FogProfile
{
    float noiseScale = 0.03f;
    float noiseIntensity = 0.0f;
    float heightDensity = 0.0f;
    float heightFalloff = 0.0f;
    Vector3 ambientLight = { 10.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f };
    float extinction = 0.005f;
    float erosion = 0.0f;
    float windSpeedMultiplier = 1.0f;
    float sunScattering = 13.0f; 
};

struct WeatherVisualParams
{
    // Terrain
    float metalness = 0.15f;
    float roughness = 1.00f;
    float environmentMapIntensity = 0.5f;
    float rippleSize = 0.0f;
    float normalIntensity = 1.7f;
    Vector4 color = { 175.0f / 255.0f, 255.0f / 255.0f, 166.0f / 255.0f, 1.0f };
    float emissiveIntensity = 3.5f;

    FogProfile fog;
};

// 天候が環境に与える影響のプロファイル
struct WeatherProfile
{
    float cloudThreshold;       
    float cloudShadowBrightness;
    float lightDimmer;          
    float atmosphereGlowDimmer; 
    float wetness;
    float rainIntensity;
    Vector2 windDirection;
    float windSpeed;
    float windTurbulence;
    Vector3 skyZenithColor;     
    Vector3 skyHorizonColor;    
    float skyColorBlendWeight;
    float transitionSpeed;

    WeatherVisualParams visual;
};

// プロファイル同士をブレンドする関数
inline TimeOfDayProfile BlendProfiles(const TimeOfDayProfile& a, const TimeOfDayProfile& b, float t)
{
    TimeOfDayProfile result;
    result.zenithColor = FS::Math::Lerp(a.zenithColor, b.zenithColor, t);
    result.horizonColor = FS::Math::Lerp(a.horizonColor, b.horizonColor, t);
    result.sunAtmosphereGlow = FS::Math::Lerp(a.sunAtmosphereGlow, b.sunAtmosphereGlow, t);
    result.directionalLightColor = FS::Math::Lerp(a.directionalLightColor, b.directionalLightColor, t);
    result.directionalLightIntensity = FS::Math::Lerp(a.directionalLightIntensity, b.directionalLightIntensity, t);
    result.ambientSkyColor = FS::Math::Lerp(a.ambientSkyColor, b.ambientSkyColor, t);
    result.ambientGroundColor = FS::Math::Lerp(a.ambientGroundColor, b.ambientGroundColor, t);
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

    // 時間・天候・霧などの設定を、groupName のグループから読み直す
    void LoadSettings(const std::string& groupName);

    void Update(LightManager* lightManager);
    void DebugGui();

    void Finalize();

    float GetTimeOfDay() const { return timeOfDay_; }
    // 空や光の見た目に使う時刻
    float GetVisualTimeOfDay() const { return CalculateVisualTime(timeOfDay_); }
    void SetTimeOfDay(float time) { timeOfDay_ = time; }
    // false の間は時刻を自動で進めない
    void SetTimeFlowEnabled(bool isEnabled) { isTimeFlowEnabled_ = isEnabled; }
    bool IsTimeFlowEnabled() const { return isTimeFlowEnabled_; }
    Vector3 GetSunDirection() const { return sunDirection_; }

    const TimeOfDayProfile& GetCurrentProfile() const { return currentProfile_; }

    // Skydome側から現在の天候プロファイルを取得するための関数
    const WeatherProfile& GetCurrentWeatherProfile() const { return currentWeatherProfile_; }

    // 天候を切り替え始める
    void RequestWeatherChange(WeatherState nextWeather);
    // 切り替えの途中を飛ばして、すぐにその天候にする
    void SetWeatherImmediately(WeatherState weather);

    WeatherState GetCurrentWeather() const { return currentWeather_; }
    WeatherState GetTargetWeather() const { return targetWeather_; }
    float GetWeatherTransitionProgress() const { return weatherTransitionT_; }

    // 現在の遷移状態に応じたスピードを返す
    float GetCurrentTransitionSpeed() const;

    ID3D12Resource* GetGlobalEnvironmentResource() const { return constantBuffer_.Get(); }

    const WeatherVisualParams& GetCurrentVisualParams() const { return currentVisualParams_; }

    // 現在の風のパラメータを取得するゲッター
    Vector2 GetWindDirection() const { return currentWeatherProfile_.windDirection; }
    float GetWindSpeed() const { return currentWeatherProfile_.windSpeed; }

private:
    EnvironmentManager();
    ~EnvironmentManager();

    // コピー禁止
    EnvironmentManager(const EnvironmentManager&) = delete;
    EnvironmentManager& operator=(const EnvironmentManager&) = delete;

    // 天候のEnumから対応するプロファイルを返すヘルパー関数
    WeatherProfile GetWeatherProfile(WeatherState state) const;
    // 霧の設定を ImGui に並べる
    void DrawFogSettings(const std::string& prefix);

    Engine* engine_ = nullptr;
    std::unique_ptr<PropertyBinder> binder_;
    int32_t startWeather_ = 0; 

    // パラメータ
    float timeOfDay_ = 12.0f;
    float timeSpeedMultiplier_ = 24.0f;
    // 夕方の移り変わりの時刻
    float duskStartHour_ = 17.0f;
    float sunsetHour_ = 18.0f;
    float nightHour_ = 19.0f;
    // 時刻を、夕方の移り変わりの時刻に合わせて見た目の時刻に変える
    float CalculateVisualTime(float time) const;
    bool isTimeFlowEnabled_ = true;
    Vector3 sunDirection_ = { 0.0f, -1.0f, 0.0f };

    TimeOfDayProfile profileNight_;   
    TimeOfDayProfile profileSunrise_; 
    TimeOfDayProfile profileDay_;     
    TimeOfDayProfile profileSunset_; 

    // ブレンドされた現在の結果を保存する変数
    TimeOfDayProfile currentProfile_;

    // 晴れのときの霧
    FogProfile fogDay_;
    FogProfile fogNight_;

    WeatherState currentWeather_ = WeatherState::Sunny;
    WeatherState targetWeather_ = WeatherState::Sunny;
    float weatherTransitionT_ = 1.0f; 
    float accumulatedWindTime_ = 0.0f;
    Vector2 windOffset_ = { 0.0f, 0.0f };

    WeatherProfile profileSunny_, profileRain_, profileThunder_;
    WeatherProfile currentWeatherProfile_;

    WeatherState editingWeather_ = WeatherState::Sunny; // ImGui で設定を編集している天候

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    GlobalEnvironmentData* cbData_ = nullptr;

    // 今フレームの補間済みビジュアル結果
    WeatherVisualParams currentVisualParams_;
};

}