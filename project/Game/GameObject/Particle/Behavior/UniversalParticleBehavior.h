#pragma once
#include "IParticleBehavior.h"

class UniversalParticleBehavior : public IParticleBehavior
{
public:
    void Initialize(ParticleState& particle, const ParticleConfig& config) override;

    void Update(ParticleState& particle) override;
};