#pragma once
#include "IParticleBehavior.h"

class HitEffectParticleBehavior : public IParticleBehavior
{
public:
    void Initialize(ParticleState& particle, const ParticleConfig& config) override;

    void Update(ParticleState& particle, const ParticleConfig& config) override;
};