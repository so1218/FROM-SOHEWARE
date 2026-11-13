#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Skydome.h"

namespace FS
{

// 空と太陽の光
class Environment : public GameObject
{
public:
    Environment(Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

private:
    Engine* engine_;
    std::unique_ptr<Skydome> skydome_;
};

}
