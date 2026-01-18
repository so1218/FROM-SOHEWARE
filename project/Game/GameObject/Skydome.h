#pragma once
#include "Engine.h"
#include "GameObject.h"

// 天球
class Skydome : public GameObject
{
public:
    Skydome(Engine* engine);

    GameObjectType GetType() const override { return GameObjectType::Enemy; }

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;
};

