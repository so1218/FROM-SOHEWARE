#include "pch.h"
#include "Game.h"
#include "PlayScene.h"
#include "TitleScene.h"
#include "GlobalVariables.h"
#include "AudioDevice.h"
#include "Input.h"
#include "TimeManager.h"
#include "TestSceneHori.h"
#include "ImGuiManager.h"
#include "DebugDraw.h"
#include "ProjectConfig.h"
#include "Engine.h"

using namespace FE;

Game::Game() 
    : engine_(std::make_unique<Engine>())
{
    ProjectConfig config;
    config.windowTitle = L"FROM SOHEWARE"; 
    config.width = 1280;
    config.height = 720;
    config.targetFPS = 60;

    engine_->Initialize(config);

    // シーンマネージャーの初期化
    sceneManager_.Initialize(engine_.get());

    // シーンの生成と登録
    sceneManager_.RegisterScene(SceneID::Title, std::make_unique<TitleScene>(engine_.get()));
    sceneManager_.RegisterScene(SceneID::Play, std::make_unique<PlayScene>(engine_.get()));
    sceneManager_.RegisterScene(SceneID::TestHori, std::make_unique<TestSceneHori>(engine_.get()));

    // 初期シーンを設定
#ifdef IS_DEVELOPMENT
    sceneManager_.SetInitialScene(SceneID::Play);
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
#ifdef IS_DEVELOPMENT
    // デバッグカメラの有効/無効切り替え
    if (Input::GetInstance().IsKeyTriggered(DIK_Y))
    {
        engine_->GetDebugCamera()->SetEnabled(!engine_->GetDebugCamera()->IsEnabled());
    }

    // グローバル変数更新
    GlobalVariables::GetInstance()->Update();

    // ゲーム一時停止切り替え
    if (Input::GetInstance().IsKeyTriggered(DIK_P))
    {
        auto timeManager = TimeManager::GetInstance();
        if (timeManager->IsPaused()) timeManager->Resume();
        else timeManager->Pause();
    }
#endif

    // ゲームシーン更新
    if (!TimeManager::GetInstance()->IsPaused())
    {
        sceneManager_.Update();
    }

    // 描画に使うカメラ情報
    Matrix4x4 viewMat, projMat;
    Vector3 eyePos;

    // 現在のシーンからカメラを取得
    Camera* sceneCamera = sceneManager_.GetCurrentScene()->GetActiveCamera();

#ifdef IS_DEVELOPMENT
    if (engine_->GetDebugCamera()->IsEnabled())
    {
        engine_->GetDebugCamera()->Update();

        // デバッグカメラの行列を使用
        viewMat = engine_->GetDebugCamera()->GetViewMatrix();
        projMat = engine_->GetDebugCamera()->GetProjectionMatrix();
        eyePos = engine_->GetDebugCamera()->GetCameraWorldPosition();
    }
    else 
    {
        // ゲーム内カメラの行列を使う
        viewMat = sceneCamera->GetViewMatrix();
        projMat = sceneCamera->GetProjectionMatrix();
        eyePos = sceneCamera->GetTranslation();
    }

    engine_->GetDebugGuiManager()->Update(sceneCamera);

#else
    // リリース時は常にシーンカメラ
    viewMat = sceneCamera->GetViewMatrix();
    projMat = sceneCamera->GetProjectionMatrix();
    eyePos = sceneCamera->GetTranslation();
#endif

    // 決定したカメラ情報をEngineに転送
    engine_->SetCameraState(
        viewMat, 
        projMat,
        eyePos,
        sceneCamera->GetNearClip(), 
        sceneCamera->GetFarClip());

#ifdef IS_DEVELOPMENT
    // ゲームカメラ視錐台を描画
    if (engine_->GetDebugCamera()->IsEnabled())
    {
        DebugDraw::DrawFrustum(sceneCamera->GetViewProjectionMatrix(), { 1.0f, 1.0f, 0.0f, 1.0f });
    }

    ImGuiManager::SetGizmoCamera(viewMat, projMat);

#endif
}

void Game::Draw()
{
    sceneManager_.Draw();
}

void Game::DebugDraw()
{
#ifdef IS_DEVELOPMENT
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
        if (ImGui::Button("ホリーテストシーン"))
        {
            sceneManager_.RequestSceneChange(SceneID::TestHori);
        }
    }
    ImGui::End();

    sceneManager_.DebugDraw();

    engine_->GetLightManager()->DrawDebugLights();
#endif
}

void Game::Finalize()
{
    Input::GetInstance().Finalize();
    AudioDevice::GetInstance().Finalize();
}