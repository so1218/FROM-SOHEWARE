#pragma once
#include "GameObject.h"
#include "Engine.h" 
#include "Camera.h" 

class ParticleSystemWrapper : public GameObject
{
public:
    ParticleSystemWrapper(Engine* engine);

    void Update() override;

    void Draw() override;

private:
};