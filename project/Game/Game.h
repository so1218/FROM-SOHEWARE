#pragma once
#include "SceneManager.h"
#include "DebugUtils.h"

namespace FS
{
    class Engine;
}

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
    std::unique_ptr<FS::Engine> engine_;

    // シーン管理
    FS::SceneManager sceneManager_;
};

