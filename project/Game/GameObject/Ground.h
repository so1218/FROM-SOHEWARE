#pragma once
#include "Engine.h"
#include "GameObject.h"

class Ground : public GameObject
{
public:
    Ground(Engine* engine, Camera* camera);

    GameObjectType GetType() const override { return GameObjectType::Enemy; }

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

};

