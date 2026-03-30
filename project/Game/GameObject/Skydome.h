#pragma once
#include "Engine.h"
#include "GameObject.h"

// 天球
class Skydome : public FE::GameObject
{
public:
    Skydome(FE::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;
};

