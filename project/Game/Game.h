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

    void Run();
    void Update();
    void Draw();
    void DebugGui();

private:
    std::unique_ptr<FS::Engine> engine_;
    FS::SceneManager sceneManager_;
};

