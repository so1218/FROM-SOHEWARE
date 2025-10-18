#pragma once
#include "ParticleSystem.h"

class ParticleEditor
{
public:
    ParticleEditor(ParticleSystem* system);

    void ShowEditor();

    void ApplyEmitterConfigToLiveEmitters(const std::string& presetName);

private:
    ParticleSystem* particleSystem_;
};

