#include "pch.h"
#include "MainScene.h"
#include "GameDefine.h"
#include "Grid.h"
#include "Environment.h"

using namespace FS;

MainScene::MainScene(Engine* engine)
    : BaseScene(engine, SceneName::Main)
{
    // 空と太陽の光。置かないとモデルが暗く映る
    objectManager_.Create<Environment>(engine_);
    objectManager_.Create<Grid>(engine_);
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
