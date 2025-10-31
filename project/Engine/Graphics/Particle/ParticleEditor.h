#pragma once
#include "ParticleSystem.h"

class ParticleEditor
{
public:
    ParticleEditor(ParticleSystem* system);

    void ShowEditor();

    void ApplyEmitterConfigToLiveEmitters(const std::string& presetName);

    void ResetSelection() {
        selectedPresetIdx_ = 0;
        selectedTextureIdx_ = 0;
    }

private:
    ParticleSystem* particleSystem_;

    int selectedPresetIdx_ = 0;
    int selectedTextureIdx_ = 0;
};

