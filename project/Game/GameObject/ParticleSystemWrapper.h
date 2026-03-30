#pragma once
#include "GameObject.h"
#include "Engine.h" 
#include "Camera.h" 

class ParticleSystemWrapper : public FE::GameObject
{
public:
    ParticleSystemWrapper(FE::Engine* engine);

    void Update() override;

    void Draw() override;

private:
};