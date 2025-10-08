#pragma once
#include "IParticleBehavior.h"

class KeyParticleBehavior : public IParticleBehavior
{
public:
    void Initialize(ParticleState& particle, const ParticleSystem& system) override;

    void Update(ParticleState& particle) override;
};