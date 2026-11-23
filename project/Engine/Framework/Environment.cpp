#include "pch.h"
#include "Environment.h"
#include "EnvironmentManager.h"

namespace FS
{

Environment::Environment(Engine* engine, const std::string& environmentGroupName, const std::string& skydomeGroupName)
    : GameObject(), engine_(engine)
{
    // 太陽の光や霧は、ほかのオブジェクトより先に、このシーンの設定にしておく
    EnvironmentManager::GetInstance()->LoadSettings(environmentGroupName);
    skydome_ = std::make_unique<Skydome>(engine_, skydomeGroupName);
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
