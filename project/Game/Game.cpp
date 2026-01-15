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
#include "DebugDraw.h"

using namespace FromEngine;

Game::Game() : engine_(std::make_unique<Engine>()), camera_(std::make_unique<Camera>()), materialManager_(std::make_unique<MaterialManager>())
{
    engine_->Initialize(camera_.get(), materialManager_.get());
#ifdef _DEBUG
    DebugDraw::Initialize(engine_->renderer_.get());
#endif

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
#ifdef _DEBUG
            DebugDraw();
#endif
            Draw();

            // フレームの終了
            engine_->EndFrame();
        }
    }

    Finalize();
}

void Game::Update()
{
#ifdef _DEBUG
    // デバッグカメラの有効/無効切り替え
    if (Input::GetInstance().IsKeyTriggered(DIK_Y))
    {
        engine_->debugCamera_->SetEnabled(!engine_->debugCamera_->IsEnabled());
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

    // マテリアルをグローバル状態に合わせて更新
    materialManager_->UpdateAllMaterialsFromGlobal();

    // ゲームロジック更新（シーン）
    if (!TimeManager::GetInstance()->IsPaused())
    {
        sceneManager_.Update();
    }

    // ゲームカメラの行列を更新
    camera_->UpdateViewProjectionMatrix();
    Matrix4x4 gameCameraVP = camera_->GetViewProjectionMatrix();

    // デバッグカメラが有効なら上書き
    if (engine_->debugCamera_->IsEnabled())
    {
        engine_->debugCamera_->Update();
    /*    camera_->SetTranslation(engine_->debugCamera_->GetCameraWorldPosition());*/
        camera_->SetViewMatrix(engine_->debugCamera_->GetViewMatrix());
        camera_->SetProjectionMatrix(engine_->debugCamera_->GetProjectionMatrix());
        camera_->SetViewProjectionMatrix(engine_->debugCamera_->GetViewProjectionMatrix());
    }
    else
    {
        // デバッグカメラ無効ならゲームカメラ行列を使用
        camera_->SetViewMatrix(camera_->GetViewMatrix());
        camera_->SetProjectionMatrix(camera_->GetProjectionMatrix());
        camera_->SetViewProjectionMatrix(gameCameraVP);
    }

#ifdef _DEBUG
    // デバッグ描画用カメラを設定
    DebugDraw::SetCamera(camera_.get());

    // ゲームカメラ視錐台を描画
    if (engine_->debugCamera_->IsEnabled())
        DebugDraw::DrawFrustum(gameCameraVP, { 1.0f, 1.0f, 0.0f, 1.0f });
#endif

    // シャドウマップ更新
   /* auto* dirLights = engine_->lightManager_->GetDirectionalLightData();
    if (dirLights[0].enable)
    {
        Vector3 lightDir = dirLights[0].direction.Normalize();
        Vector3 shadowTarget = { 0.0f, 0.0f, 0.0f };
        float distance = 100.0f;
        Vector3 lightPos = shadowTarget - (lightDir * distance);

        Vector3 up = { 0.0f, 1.0f, 0.0f };
        if (fabs(lightDir.y) > 0.99f) up = { 1.0f, 0.0f, 0.0f };

        Matrix4x4 lightView = Matrix4x4::MakeLookAt(lightPos, shadowTarget, up);
        Matrix4x4 lightProj = Matrix4x4::MakeOrthographic(30.0f, 30.0f, -100.0f, 200.0f);
        Matrix4x4 lightViewProj = lightView * lightProj;

        engine_->lightManager_->UpdateDirectionalLightShadowMatrix(0, lightViewProj);
    }*/
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

    engine_->lightManager_->DrawDebugLights();
}

void Game::Finalize()
{
    Input::GetInstance().Finalize();
    AudioManager::GetInstance().Finalize();
}