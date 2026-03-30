#pragma once
#include "WorldTransform.h"

namespace FE
{

class Engine;
class Camera; 

class Skybox
{
public:
    Skybox(Engine* engine);

    void Draw();

    void SetCubeTexture(const std::string& textureName);

    void SetColor(uint32_t color);

    WorldTransform& GetTransform();

private:
    Engine* engine_ = nullptr;

    WorldTransform transform_; 
    uint32_t cubeTextureHandle_ = 0; 
    uint32_t color_ = 0xFFFFFFFF;
};

}
