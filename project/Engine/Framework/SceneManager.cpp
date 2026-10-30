#include "pch.h"
#include "SceneManager.h"
#include "Engine.h"
#include "BaseScene.h"
#include "NormalState.h"
#include "FadeInState.h"
#include "ISceneTransitionState.h"
#include "Fade.h"
#include "Logger.h"
#include "ImGuiManager.h"

namespace FS
{

SceneManager::SceneManager() : currentScene_(nullptr)
{}

SceneManager::~SceneManager()
{
    // 最後に有効だったシーンのFinalizeを呼ぶ
    if (currentScene_)
    {
        currentScene_->Finalize();
    }
}

void SceneManager::Initialize(Engine* engine)
{
    engine_ = engine;
    fade_ = std::make_unique<Fade>(engine_);
    fade_->Initialize();

    state_ = std::make_unique<NormalState>();
}

void SceneManager::Update()
{
    if (state_)
    {
        state_->Update(this);
    }
}

void SceneManager::Draw()
{
    if (state_)
    {
        state_->Draw(this);
    }
}

void SceneManager::DebugDraw()
{
    if (!ImGuiManager::IsGuiVisible()) return;

    if (currentScene_)
    {
        currentScene_->DebugDraw();
    }
	fade_->DebugDraw();
}

// シーンをマップに登録する
void SceneManager::RegisterScene(std::unique_ptr<BaseScene> scene)
{
    const std::string& name = scene->GetSceneName();
    assert(!scenes_.contains(name) && "同じ名前のシーンが二重に登録されている");

    scene->SetSceneManager(this);
    sceneOrder_.push_back(name);
    scenes_[name] = std::move(scene);
}

BaseScene* SceneManager::FindScene(const std::string& sceneName) const
{
    auto it = scenes_.find(sceneName);
    return (it != scenes_.end()) ? it->second.get() : nullptr;
}

void SceneManager::SetInitialScene(const std::string& sceneName)
{
    BaseScene* scene = FindScene(sceneName);
    assert(scene && "SetInitialScene: 登録されていないシーン名");  

    SetScene(scene);
    ChangeState(std::make_unique<FadeInState>());
    fade_->Start(Fade::Status::FadeIn, fade_->GetDuration());
}

void SceneManager::RequestSceneChange(const std::string& sceneName)
{
    if (!FindScene(sceneName))
    {
        LOG_WARN("RequestSceneChange: 登録されていないシーン名: {}", sceneName);
        return;   
    }
    nextSceneName_ = sceneName;
}


// 内部用のシーン設定処理
void SceneManager::SetScene(BaseScene* newScene)
{
    // 古いシーンがあれば終了処理
    if (currentScene_)
    {
        currentScene_->Finalize();
    }

    engine_->GetParticleSystem()->Clear();
    currentScene_ = newScene;

    // 新しいシーンの初期化処理
    if (currentScene_)
    {
        currentScene_->Initialize();
    }
}

// 状態を切り替える関数
void SceneManager::ChangeState(std::unique_ptr<ISceneTransitionState> newState)
{
    state_ = std::move(newState);
}

void SceneManager::ChangeSceneActual()
{
    if (nextSceneName_)
    {
        if (BaseScene* scene = FindScene(*nextSceneName_))
        {
            SetScene(scene);
        }
        nextSceneName_ = std::nullopt;
    }
}

}