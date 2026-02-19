#include "ParticleSystemWrapper.h" 

ParticleSystemWrapper::ParticleSystemWrapper(Engine* engine)
    : GameObject(engine)
{
	SetTag("ParticleSystemWrapper");
}

void ParticleSystemWrapper::Update()
{
    engine_->particleSystem_->Update();
}

void ParticleSystemWrapper::Draw()
{
    engine_->particleSystem_->Draw();
}
