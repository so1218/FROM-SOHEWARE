#pragma once
#include "WorldTransform.h"
#include "Structures.h"

namespace FS
{

class Engine;
class PropertyBinder;

class Skydome
{
public:
    Skydome(Engine* engine);
    ~Skydome();

    void Initialize(); 
    void Update();
    void Draw();
    void DebugGui();  

    void SetCloudNoiseTexture(const std::string& textureName);

private:
    Engine* engine_ = nullptr;
    std::unique_ptr<PropertyBinder> binder_;

    uint32_t cloudNoiseHandle_ = 0;

    AtmosphereSkyData atmosphereSkyData_{};
};

}