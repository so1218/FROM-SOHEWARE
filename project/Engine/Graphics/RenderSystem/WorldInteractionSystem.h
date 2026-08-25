#pragma once
#include "WorldTransform.h"
#include "Structures.h"

namespace FE
{

class Engine;
class Terrain;
class PropertyBinder;

class WorldInteractionSystem
{
public:
    WorldInteractionSystem(FE::Engine* engine);
    ~WorldInteractionSystem() = default;

    void Initialize();
    void Update();
    void Draw();
    void DebugDraw();

    // 地形オブジェクトの参照を設定
    void SetTerrain(FE::Terrain* terrain) { terrain_ = terrain; }

    const InteractionConstants& GetConstants() const { return constants_; }

private:
    Engine* engine_ = nullptr;
    Terrain* terrain_ = nullptr;
    std::unique_ptr<PropertyBinder> binder_;

    InteractionConstants constants_{};
};

}