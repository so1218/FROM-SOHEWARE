#pragma once
#include "WorldTransform.h"

namespace FE
{

class Engine;
class Camera;

class Skydome
{
public:
    Skydome(Engine* engine);

    void Draw();

    void SetSkyCubeTexture(const std::string& textureName);
    void SetCloudNoiseTexture(const std::string& textureName);
    void SetColor(uint32_t color);

    WorldTransform& GetTransform();

private:
    Engine* engine_ = nullptr;

    WorldTransform transform_;
    uint32_t skyCubeHandle_ = 0;     // t0: 空用キューブマップ
    uint32_t cloudNoiseHandle_ = 0;  // t1: 雲用2Dノイズ
    uint32_t color_ = 0xFFFFFFFF;
};

}