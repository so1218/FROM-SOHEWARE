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

    void Initialize(); 
    void Update();
    void Draw();
    void DebugDraw();  

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
    AtmosphereSkyData atmosphereSkyData_;
};

}