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
    // 初期シーンを設定
    sceneManager_.SetScene(std::make_unique <PlayScene> (engine_.get(), camera_.get()));
}

Game::~Game()
{
    sceneManager_.Finalize();
    // ライブラリの終了
    engine_->Finalize();
}

void Game::Initialize()
{
    modelDataGrid_ = ModelHandle::Get(ModelID::field);
    worldTransformGrid_.scale_ = { 10000.0f, 1.0f,10000.0f };
    worldTransformGrid_.UpdateMatrix();
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
            Input::Update();
            // フレームの開始
            engine_->BeginFrame();
            
            // 描画前処理
            engine_->PreDraw();

            Update();
            Draw();

            // フレームの終了
            engine_->EndFrame();
        }
    }

    Finalize();
}

void Game::Update()
{
    if (Input::IsKeyTriggered(DIK_Y))
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
#endif

    // ポーズボタン押下判定
    if (Input::IsKeyTriggered(DIK_P))
    {
        auto timeManager = TimeManager::GetInstance();
        if (timeManager->IsPaused())
            timeManager->Resume();
        else
            timeManager->Pause();
    }

	materialManager_->UpdateAllMaterialsFromGlobal();
    modelDataGrid_->materialHandle.materialData->isArtGrid = true;

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
#ifdef _DEBUG
    engine_->DrawGrid(worldTransformGrid_, *camera_, *modelDataGrid_, TextureHandle::Get(TextureID::white1x1), 0xffffff00);
#endif

    sceneManager_.Draw();
#ifdef _DEBUG
    DebugDraw();
#endif

}

void Game::DebugDraw()
{
    if (ImGui::Begin("シーンの選択"))
    {
        if (ImGui::Button("Title Scene"))
        {
            sceneManager_.RequestSceneChange(std::make_unique<TitleScene>(engine_.get(), camera_.get()));
        }
        if (ImGui::Button("Play Scene"))
        {
            sceneManager_.RequestSceneChange(std::make_unique<PlayScene>(engine_.get(), camera_.get()));
        }
        if (ImGui::Button("Sample Scene"))
        {
            sceneManager_.RequestSceneChange(std::make_unique<SampleSceneHori>(engine_.get(), camera_.get()));
        }
    }
    ImGui::End();
}

void Game::Finalize()
{
    sceneManager_.Finalize();
    AudioManager::GetInstance().Finalize();
}