#include "pch.h"
#include "Game.h"
#include "GameDefine.h"
#include "PlayScene.h"
#include "TestScene.h"
#include "Input.h"
#include "TimeManager.h"
#include "ImGuiManager.h"
#include "ProjectConfig.h"
#include "Engine.h"

using namespace FS;

Game::Game() 
    : engine_(std::make_unique<Engine>())
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

    engine_->Initialize(config);

    // シーンマネージャーの初期化
    sceneManager_.Initialize(engine_.get());

    // シーンの生成と登録
    sceneManager_.RegisterScene(std::make_unique<PlayScene>(engine_.get()));
    sceneManager_.RegisterScene(std::make_unique<TestScene>(engine_.get()));

    // 初期シーンを設定
    sceneManager_.SetInitialScene(SceneName::Play);
}

Game::~Game()
{
    // シーンはエンジンを使うので、先に片付ける
    sceneManager_.Finalize();
    engine_->Finalize();
}

void Game::Run()
{
	MSG msg{};
    // ウィンドウのxボタンが押されるまでのループ
    while (msg.message != WM_QUIT)
    {
        // Windowにメッセージが来てたら最優先で処理させる
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // キーボード情報の取得開始
            Input::GetInstance().Update();
            // フレームの開始
            engine_->BeginFrame();

            Update();

            // 調整用の画面はここでだけ切り替える
#ifdef ENABLE_IMGUI
            DebugGui();
#endif

            Draw();

            // フレームの終了
            engine_->EndFrame();
        }
    }
}

void Game::Update()
{
    if (!TimeManager::GetInstance()->IsPaused())
    {
        sceneManager_.Update();
    }

    BaseScene* scene = sceneManager_.GetCurrentScene();
    if (!scene) return;

    engine_->SetSceneCamera(scene->GetActiveCamera());
}

void Game::Draw()
{
    sceneManager_.Draw();

#ifdef ENABLE_DEBUG_DRAW
    engine_->GetLightManager()->DrawDebugLights();
#endif
}

void Game::DebugGui()
{
    if (ImGui::Begin("シーンの選択"))
    {
        for (const std::string& name : sceneManager_.GetSceneNames())
        {
            if (ImGui::Button(name.c_str()))
            {
                sceneManager_.RequestSceneChange(name);
            }
        }
    }
    ImGui::End();

    sceneManager_.DebugGui();
}
