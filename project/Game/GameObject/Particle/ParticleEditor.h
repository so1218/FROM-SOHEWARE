#pragma once
#include "Particle.h"

class ParticleEditor
{
public:
    ParticleEditor(ParticleSystem* system);

    void ShowEditor();

    void ApplyEmitterConfigToLiveEmitters(ParticleType type, const std::string& presetName);

private:
    ParticleSystem* particleSystem_;
};

