#pragma once
#include "WorldTransform.h"
#include "Structures.h"

namespace FS
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

    WorldTransform& GetTransform();

private:
    Engine* engine_ = nullptr;
    std::unique_ptr<PropertyBinder> binder_;

    WorldTransform transform_;
    uint32_t cloudNoiseHandle_ = 0;

    // 雲の形はここで調整し、空の色・雲の量・太陽の向きは EnvironmentManager から毎フレーム受け取る
    AtmosphereSkyData atmosphereSkyData_{};
};

}