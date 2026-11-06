#pragma once
#include "GameObject.h"
#include "Engine.h" 
#include "Camera.h" 

class ParticleSystemWrapper : public FS::GameObject
{
public:
    ParticleSystemWrapper(FS::Engine* engine);

    void Update() override;

    void Draw() override;
    void DebugGui() override;

private:
    FS::Engine* engine_;
};