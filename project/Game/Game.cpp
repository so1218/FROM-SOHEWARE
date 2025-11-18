#include "Game.h"
#include "PlayScene.h"
#include "TitleScene.h"
#include "GlobalVariables.h"
#include "AudioManager.h"
#include "Input.h"
#include "TimeManager.h"
#include "ModelHandle.h"
#include "TextureHandle.h"
#include "SampleSceneHori.h"
#include "ImGuiManager.h"


Game::Game() : engine_(std::make_unique<Engine>()), camera_(std::make_unique<Camera>()), materialManager_(std::make_unique<MaterialManager>())
{
    engine_->Initialize(camera_.get(), materialManager_.get());

    // シーンマネージャーの初期化
    sceneManager_.Initialize(engine_.get());

    // シーンの生成と登録
    sceneManager_.RegisterScene(SceneID::Title, std::make_unique<TitleScene>(engine_.get(), camera_.get()));
    sceneManager_.RegisterScene(SceneID::Play, std::make_unique<PlayScene>(engine_.get(), camera_.get()));
    sceneManager_.RegisterScene(SceneID::Sample, std::make_unique<SampleSceneHori>(engine_.get(), camera_.get()));

    // 初期シーンを設定
#ifdef _DEBUG
    sceneManager_.SetInitialScene(SceneID::Sample);
#else
    sceneManager_.SetInitialScene(SceneID::Title);
#endif
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
            Draw();
#ifdef _DEBUG
            DebugDraw();
#endif

            // フレームの終了
            engine_->EndFrame();
        }
    }

    Finalize();
}

void Game::Update()
{
#ifdef _DEBUG
    if (Input::GetInstance().IsKeyTriggered(DIK_Y))
    {
        if (engine_->debugCamera_->IsEnabled())
        {
            engine_->debugCamera_->SetEnabled(false);
        }
        else
        {
            engine_->debugCamera_->SetEnabled(true);
        }
    }
#endif

    if (!engine_->debugCamera_->IsEnabled())
    {
        camera_->UpdateViewProjectionMatrix();
        camera_->SetViewMatrix(camera_->GetViewMatrix());
        camera_->SetProjectionMatrix(camera_->GetProjectionMatrix());
        camera_->SetViewProjectionMatrix(camera_->GetViewProjectionMatrix());
    }

#ifdef _DEBUG
    // グローバル変数の更新
    GlobalVariables::GetInstance()->Update();

    // ポーズボタン押下判定
    if (Input::GetInstance().IsKeyTriggered(DIK_P))
    {
        auto timeManager = TimeManager::GetInstance();
        if (timeManager->IsPaused())
            timeManager->Resume();
        else
            timeManager->Pause();
    }
#endif

	materialManager_->UpdateAllMaterialsFromGlobal();

    if (!TimeManager::GetInstance()->IsPaused())
    {
        sceneManager_.Update();
    }

    if (engine_->debugCamera_->IsEnabled())
    {
        // デバッグカメラを更新
        engine_->debugCamera_->Update();

        // camera_にコピー
        camera_->SetViewMatrix(engine_->debugCamera_->GetViewMatrix());
        camera_->SetProjectionMatrix(engine_->debugCamera_->GetProjectionMatrix());
        camera_->SetViewProjectionMatrix(engine_->debugCamera_->GetViewProjectionMatrix());
    }
}

void Game::Draw()
{
    sceneManager_.Draw();
}

void Game::DebugDraw()
{
    if (ImGui::Begin("シーンの選択"))
    {
        if (ImGui::Button("タイトルシーン"))
        {
            sceneManager_.RequestSceneChange(SceneID::Title);
        }
        if (ImGui::Button("プレイシーン"))
        {
            sceneManager_.RequestSceneChange(SceneID::Play);
        }
        if (ImGui::Button("サンプルシーン"))
        {
            sceneManager_.RequestSceneChange(SceneID::Sample);
        }
    }
    ImGui::End();

    sceneManager_.DebugDraw();
}

void Game::Finalize()
{
    Input::GetInstance().Finalize();
    AudioManager::GetInstance().Finalize();
}