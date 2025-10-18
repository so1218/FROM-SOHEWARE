#pragma once

#pragma once
#include "Particle.h" 

class ParticleConfigManager
{
public:
    ParticleConfigManager(ParticleSystem* system);

    // 読み込み処理：ParticleSystemが持つdefinitions_を引数で受け取る
    void LoadParticleDefinitionFromJson(ParticleType type);

    // 保存処理：同様に、保存したいdefinitions_を引数で受け取る
    void SaveConfigToJson(ParticleType type);

private:

    ParticleSystem* particleSystem_;
};