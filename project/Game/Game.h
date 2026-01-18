#pragma once
#include "Engine.h"
#include "DebugCamera.h"
#include "SceneManager.h"
#include "DebugLayerManager.h"
#include "DebugUtils.h"

class Game
{
public:
    Game();
    ~Game();

    void Initialize();
    void Run();
    void Update();
    void Draw();
    void DebugDraw();
    void Finalize();

private:
    // エンジン
    std::unique_ptr<Engine> engine_;

    // シーン管理
    SceneManager sceneManager_;
};

