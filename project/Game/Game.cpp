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

    {
        // 1. ライト情報の取得
        // (ここでは0番目のディレクショナルライトを使うと仮定)
        auto* dirLights = engine_->lightManager_->GetDirectionalLightData();
        if (dirLights[0].enable) // 有効なら計算
        {
            Vector3 lightDir = dirLights[0].direction;
            lightDir = lightDir.Normalize();

            // 2. ビュー行列 (View) の計算
            // 影を落としたい中心座標（プレイヤーなど）
            // デバッグ中は原点 {0,0,0} でもOKですが、プレイヤーに追従させると高品質になります
            Vector3 shadowTarget = { 0.0f, 0.0f, 0.0f };

            float distance = 100.0f; // 光源までの距離
            Vector3 lightPos = shadowTarget - (lightDir * distance);

            Vector3 up = { 0.0f, 1.0f, 0.0f };
            if (fabs(lightDir.y) > 0.99f) up = { 1.0f, 0.0f, 0.0f }; // 真上/真下対策

            Matrix4x4 lightView = Matrix4x4::MakeLookAt(lightPos, shadowTarget, up);

            // 3. プロジェクション行列 (Projection) の計算
            // 平行光源なので正射影を使います
            float size = 100.0f; // 影が落ちる範囲（幅・高さ）
            float nearZ = 0.1f;
            float farZ = 200.0f; // 深度の範囲 (distanceより大きく)

            Matrix4x4 lightProj = Matrix4x4::MakeOrthographic(size, size, nearZ, farZ);

            // 4. 行列合成 & 更新
            Matrix4x4 lightViewProj = lightView * lightProj;

            // ★ここで呼び出す！
            engine_->lightManager_->UpdateDirectionalLightShadowMatrix(0, lightViewProj);
        }
    }

    if (engine_->debugCamera_->IsEnabled())
    {
        // デバッグカメラを更新
        engine_->debugCamera_->Update();

        // camera_にコピー
        camera_->SetTranslation(engine_->debugCamera_->GetCameraWorldPosition());
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