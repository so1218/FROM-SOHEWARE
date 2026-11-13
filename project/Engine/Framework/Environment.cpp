#include "pch.h"
#include "Environment.h"
#include "EnvironmentManager.h"

namespace FS
{

Environment::Environment(Engine* engine) : GameObject(), engine_(engine)
{
    skydome_ = std::make_unique<Skydome>(engine_);
}

void Environment::Initialize()
{
    skydome_->Initialize();
}

void Environment::Update()
{
    EnvironmentManager::GetInstance()->Update(engine_->GetLightManager());
    skydome_->Update();
}

void Environment::Draw()
{
    skydome_->Draw();
}

void Environment::DebugGui()
{
    EnvironmentManager::GetInstance()->DebugGui();
    skydome_->DebugGui();
}

}
