#pragma once
#include "WorldTransform.h"
#include "Structures.h"

namespace FE
{

class Engine;
class Camera;
class PropertyBinder;

class Skydome
{
public:
    Skydome(Engine* engine);
    ~Skydome() = default;

    void Initialize(); // 初期化処理をまとめる
    void Update();
    void Draw();
    void DebugDraw();  // ImGui描画用

    void SetCloudNoiseTexture(const std::string& textureName);
    void SetColor(uint32_t color);

    WorldTransform& GetTransform();

private:
    Engine* engine_ = nullptr;
    std::unique_ptr<PropertyBinder> binder_;

    WorldTransform transform_;
    uint32_t cloudNoiseHandle_ = 0;
    uint32_t color_ = 0xFFFFFFFF;

    // 天候パラメータ
    WeatherData weatherData_;
};

}