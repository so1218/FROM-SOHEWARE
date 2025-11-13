#pragma once
#include "Engine.h"
#include "DebugCamera.h"
#include "SceneManager.h"
#include "WorldTransform.h"
#include "Model.h"
#include "Grid.h"
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
    // メインエンジンとカメラ
    std::unique_ptr<Engine> engine_;
    std::unique_ptr<Camera> camera_;

    // マテリアル管理
    std::unique_ptr<MaterialManager> materialManager_;

    // シーン管理
    SceneManager sceneManager_;
};

