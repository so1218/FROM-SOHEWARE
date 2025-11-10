#pragma once
#include "GameObject.h"
#include "Engine.h" 
#include "Camera.h" 

class ParticleSystemWrapper : public GameObject
{
public:
    ParticleSystemWrapper(Engine* engine, Camera* camera);

    void Update() override;

    void Draw() override;

    GameObjectType GetType() const override { return GameObjectType::Effect; }

    std::vector<std::string> GetGlobalVariableGroupName() const { return { "ParticleSystem" }; }

private:
    Engine* engine_ = nullptr;
    Camera* camera_ = nullptr;
};