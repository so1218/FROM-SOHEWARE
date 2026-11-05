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

    // 開発用ツールのない Release だけフルスクリーン
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
    // ライブラリの終了
    engine_->Finalize();
}

void Game::Initialize()
{
}

void Game::Run()
{
	MSG msg{};
    Initialize();
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

            DebugDraw();

            Draw();

            // フレームの終了
            engine_->EndFrame();
        }
    }

    Finalize();
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
}

void Game::DebugDraw()
{
#ifdef ENABLE_IMGUI
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
#endif

    sceneManager_.DebugDraw();

#ifdef ENABLE_DEBUG_DRAW
    engine_->GetLightManager()->DrawDebugLights();
#endif
}

void Game::Finalize()
{
}