#pragma once
#include "SceneManager.h"
#include "DebugUtils.h"

namespace FE
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
    std::unique_ptr<FE::Engine> engine_;

    // シーン管理
    FE::SceneManager sceneManager_;
};

