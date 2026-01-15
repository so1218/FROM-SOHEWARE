#include "ParticleSystemWrapper.h" 

ParticleSystemWrapper::ParticleSystemWrapper(Engine* engine, Camera* camera)
    : GameObject(engine, camera)
{
}

void ParticleSystemWrapper::Update()
{
    engine_->particleSystem_->Update();
}

void ParticleSystemWrapper::Draw()
{
    engine_->particleSystem_->Draw(camera_);
}
