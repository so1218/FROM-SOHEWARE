#pragma once
#include "ParticleDefinition.h" 

class ParticleSystem;

class IParticleBehavior 
{
public:
    virtual ~IParticleBehavior() = default;
    virtual void Initialize(ParticleState& particle, const ParticleConfig& config) = 0;
    virtual void Update(ParticleState& particle) = 0;
};