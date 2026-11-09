#include "pch.h"
#include "Game.h"
#include "GameDefine.h"
#include "PlayScene.h"
#include "TestScene.h"
#include "ProjectConfig.h"

using namespace FS;

namespace
{

ProjectConfig MakeConfig()
{
    ProjectConfig config;
    config.windowTitle = L"LAST BLOOD: A War Alone";
    config.width = 1280;
    config.height = 720;
    config.targetFPS = 60;

    // Release
#ifndef ENABLE_DEV_TOOLS
    config.isFullscreen = true;
#endif

    return config;
}

}

Game::Game()
    : Application(MakeConfig())
{
    // シーンの生成と登録
    GetSceneManager().RegisterScene(std::make_unique<PlayScene>(GetEngine()));
    GetSceneManager().RegisterScene(std::make_unique<TestScene>(GetEngine()));

    // 初期シーンを設定
    GetSceneManager().SetInitialScene(SceneName::Play);
}
