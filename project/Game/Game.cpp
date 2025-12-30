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
            Draw();
#ifdef _DEBUG
            DebugDraw();
#endif

            // フレームの終了
            engine_->EndFrame();
        }
    }

    Finalize();
}void Game::Update()
{
    // ---------------------------------------------------------
    // 0. 前処理 (Input更新などはRunで行われている前提)
    // ---------------------------------------------------------
#ifdef _DEBUG
    if (Input::GetInstance().IsKeyTriggered(DIK_Y))
    {
        bool isEnabled = engine_->debugCamera_->IsEnabled();
        engine_->debugCamera_->SetEnabled(!isEnabled);
    }

    GlobalVariables::GetInstance()->Update();

    if (Input::GetInstance().IsKeyTriggered(DIK_P))
    {
        auto timeManager = TimeManager::GetInstance();
        if (timeManager->IsPaused()) timeManager->Resume();
        else timeManager->Pause();
    }
#endif

    materialManager_->UpdateAllMaterialsFromGlobal();


    // ---------------------------------------------------------
    // 1. ★最優先★ ゲームロジック（シーン）の更新
    // ---------------------------------------------------------
    // ここでプレイヤーが動き、ゲームカメラ(camera_)の位置調整が行われる
    if (!TimeManager::GetInstance()->IsPaused())
    {
        sceneManager_.Update();
    }


    // ---------------------------------------------------------
    // 2. ゲームカメラ本来の行列を確定・保存
    // ---------------------------------------------------------
    // シーン更新で移動した camera_ の座標を元に行列を作る
    camera_->UpdateViewProjectionMatrix();

    // この時点での「本来のゲーム視点」を保存（視錐台描画用）
    Matrix4x4 gameCameraVP = camera_->GetViewProjectionMatrix();


    // ---------------------------------------------------------
    // 3. 描画用カメラの最終決定（デバッグカメラによる上書き）
    // ---------------------------------------------------------
    // シーン更新が終わった後に上書きすることで、確実にデバッグ視点になる
    if (engine_->debugCamera_->IsEnabled())
    {
        engine_->debugCamera_->Update();

        camera_->SetTranslation(engine_->debugCamera_->GetCameraWorldPosition());
        camera_->SetViewMatrix(engine_->debugCamera_->GetViewMatrix());
        camera_->SetProjectionMatrix(engine_->debugCamera_->GetProjectionMatrix());
        camera_->SetViewProjectionMatrix(engine_->debugCamera_->GetViewProjectionMatrix());
    }
    else
    {
        // デバッグカメラが無効なら、手順2で作った行列をそのまま使う
        // (念のためセットし直すが、値は変わらない)
        camera_->SetViewMatrix(camera_->GetViewMatrix());
        camera_->SetProjectionMatrix(camera_->GetProjectionMatrix());
        camera_->SetViewProjectionMatrix(gameCameraVP);
    }


    // ---------------------------------------------------------
    // 4. DebugDrawの設定と描画登録
    // ---------------------------------------------------------
#ifdef _DEBUG
    // 最終決定したカメラ視点をDebugDrawに教える
    DebugDraw::SetCamera(camera_.get());

    // デバッグカメラ有効時のみ、ゲームカメラの視錐台を描画
    if (engine_->debugCamera_->IsEnabled())
    {
        DebugDraw::DrawFrustum(gameCameraVP, { 1.0f, 1.0f, 0.0f, 1.0f });
    }
#endif


    // ---------------------------------------------------------
    // 5. シャドウマップ計算 (レンダリング直前に行う)
    // ---------------------------------------------------------
    {
        auto* dirLights = engine_->lightManager_->GetDirectionalLightData();
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
        }
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

    engine_->lightManager_->DrawDebugLights();
}

void Game::Finalize()
{
    Input::GetInstance().Finalize();
    AudioManager::GetInstance().Finalize();
}