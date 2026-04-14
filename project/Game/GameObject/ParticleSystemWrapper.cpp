#include "pch.h"
#include "ParticleSystemWrapper.h" 

using namespace FE;

ParticleSystemWrapper::ParticleSystemWrapper(Engine* engine)
    : GameObject()
{
    engine_ = engine;
}

void ParticleSystemWrapper::Update()
{
    engine_->GetParticleSystem()->Update();
}

void ParticleSystemWrapper::Draw()
{
    engine_->GetParticleSystem()->Draw();
}
