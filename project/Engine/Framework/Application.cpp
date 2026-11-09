#include "pch.h"
#include "Application.h"
#include "Engine.h"
#include "BaseScene.h"
#include "Input.h"
#include "TimeManager.h"
#include "ProjectConfig.h"

namespace FS
{

Application::Application(const ProjectConfig& config)
    : engine_(std::make_unique<Engine>())
{
    engine_->Initialize(config);
    sceneManager_.Initialize(engine_.get());
}

Application::~Application()
{
    // シーンはエンジンを使うので、先に片付ける
    sceneManager_.Finalize();
    engine_->Finalize();
}

void Application::Run()
{
    MSG msg{};
    // ウィンドウが閉じられるか、終了を要求されるまでのループ
    while (msg.message != WM_QUIT && !engine_->IsQuitRequested())
    {
        // ウィンドウにメッセージが来ていたら最優先で処理する
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        Input::GetInstance().Update();
        engine_->BeginFrame();

        Update();

        // 調整用の画面はここでだけ切り替える
#ifdef ENABLE_IMGUI
        DebugGui();
#endif

        Draw();

        engine_->EndFrame();
    }
}

void Application::Update()
{
    if (!TimeManager::GetInstance()->IsPaused())
    {
        sceneManager_.Update();
    }

    BaseScene* scene = sceneManager_.GetCurrentScene();
    if (!scene) return;

    engine_->SetSceneCamera(scene->GetActiveCamera());
}

void Application::DebugGui()
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

void Application::Draw()
{
    sceneManager_.Draw();
}

}
