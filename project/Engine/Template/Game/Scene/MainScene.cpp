#include "pch.h"
#include "MainScene.h"
#include "GameDefine.h"
#include "Grid.h"

using namespace FS;

MainScene::MainScene(Engine* engine)
    : BaseScene(engine, SceneName::Main)
{
    objectManager_.AddObject(std::make_unique<Grid>(engine_));
}

void MainScene::OnInitialize()
{
    camera_->Initialize();
    camera_->SetTranslation(Vector3(0.0f, 2.0f, -10.0f));
}

void MainScene::OnUpdate()
{
}

void MainScene::OnDraw()
{
}

void MainScene::OnDebugGui()
{
}

void MainScene::OnFinalize()
{
}
