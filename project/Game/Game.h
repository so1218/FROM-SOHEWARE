#pragma once
#include "Engine.h"
#include "DebugCamera.h"
#include "SceneManager.h"
#include "WorldTransform.h"
#include "Model.h"
#include "Grid.h"
#include "DebugLayerManager.h"

struct D3DResourceLeakChecker
{
    ~D3DResourceLeakChecker()
    {
        // リソースリーク確認
        Microsoft::WRL::ComPtr <IDXGIDebug1> debug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
            debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
            debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
        }
    }
};

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

