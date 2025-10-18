#pragma once
#include "Particle.h"

class ParticleEditor
{
public:
    ParticleEditor(ParticleSystem* system);

    void ShowEditor();

private:
    ParticleSystem* particleSystem_;
};

