#include "pch.h"
#include "Game.h"
#include "GameDefine.h"
#include "MainScene.h"
#include "ProjectConfig.h"

using namespace FS;

namespace
{

ProjectConfig MakeConfig()
{
    ProjectConfig config;
    config.windowTitle = L"New Game";
    config.width = 1280;
    config.height = 720;
    config.targetFPS = 60;

#ifndef ENABLE_DEV_TOOLS
    // Release は全画面で起動する
    config.isFullscreen = true;
#endif

    return config;
}

}

Game::Game()
    : Application(MakeConfig())
{
    // シーンを足したら、ここで登録する
    GetSceneManager().RegisterScene(std::make_unique<MainScene>(GetEngine()));

    GetSceneManager().SetInitialScene(SceneName::Main);
}
