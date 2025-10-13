#include "SceneManager.h"

void SceneManager::SetScene(std::unique_ptr<BaseScene> newScene)
{
    if (currentScene_)
    {
        currentScene_->Finalize();
    }

    // SceneManagerをシーンに渡す
    newScene->SetSceneManager(this);

    currentScene_ = std::move(newScene);
    currentScene_->Initialize();
}

void SceneManager::Update()
{
    // シーン切り替えがあれば実行
    if (nextScene_)
    {
        SetScene(std::move(nextScene_));
    }

    if (currentScene_)
    {
        currentScene_->Update();
    }
}

void SceneManager::Draw()
{
    if (currentScene_)
    {
#ifdef _DEBUG
        currentScene_->DebugDraw(); 
#endif

        currentScene_->Draw();
    }
}

void SceneManager::Initialize()
{
    if (currentScene_)
    {
        currentScene_->Initialize();
    }
}

void SceneManager::Finalize()
{
    if (currentScene_)
    {
        currentScene_->Finalize();
    }
}