#include "pch.h"
#include "ParticleSystemWrapper.h" 

ParticleSystemWrapper::ParticleSystemWrapper(Engine* engine)
    : GameObject(engine)
{
	SetTag("ParticleSystemWrapper");
}

void ParticleSystemWrapper::Update()
{
    engine_->GetParticleSystem()->Update();
}

void ParticleSystemWrapper::Draw()
{
    engine_->GetParticleSystem()->Draw();
}
