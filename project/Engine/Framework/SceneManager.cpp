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
#include "AudioPlayer.h"

namespace FS
{

SceneManager::SceneManager() : currentScene_(nullptr)
{}

SceneManager::~SceneManager()
{
    Finalize();
}

void SceneManager::Finalize()
{
    if (currentScene_)
    {
        currentScene_->Finalize();
        currentScene_ = nullptr;
    }

    state_.reset();
    retiredScene_.reset();
    scenes_.clear();
    sceneFactories_.clear();
    fade_.reset();
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
    // 前のフレームの描画は GPU が終えているので、作り直す前のシーンをここで捨てる
    retiredScene_.reset();

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

void SceneManager::DebugGui()
{
    if (!ImGuiManager::IsGuiVisible()) return;

    if (currentScene_)
    {
        currentScene_->DebugGui();
    }
	fade_->DebugGui();
}

void SceneManager::RegisterScene(std::function<std::unique_ptr<BaseScene>()> factory)
{
    std::unique_ptr<BaseScene> scene = factory();
    sceneFactories_[scene->GetSceneName()] = std::move(factory);
    RegisterScene(std::move(scene));
}

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
    isReloadRequested_ = false;
}

void SceneManager::RequestSceneReload()
{
    if (!currentScene_)
    {
        return;
    }

    const std::string& sceneName = currentScene_->GetSceneName();
    if (!sceneFactories_.contains(sceneName))
    {
        LOG_WARN("RequestSceneReload: 作る関数を登録していないシーンは作り直せない: {}", sceneName);
        return;
    }
    nextSceneName_ = sceneName;
    isReloadRequested_ = true;
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
    AudioPlayer::GetInstance().InvalidateListener();
    currentScene_ = newScene;

    // 新しいシーンの初期化処理
    if (currentScene_)
    {
        currentScene_->Initialize();
    }
}

void SceneManager::ReloadScene(const std::string& sceneName)
{
    auto factory = sceneFactories_.find(sceneName);
    auto scene = scenes_.find(sceneName);
    if (factory == sceneFactories_.end() || scene == scenes_.end())
    {
        return;
    }

    // 古いシーンの終わりの処理は、新しいシーンを作る前に済ませる
    if (currentScene_)
    {
        currentScene_->Finalize();
        currentScene_ = nullptr;
    }
    retiredScene_ = std::move(scene->second);

    std::unique_ptr<BaseScene> newScene = factory->second();
    newScene->SetSceneManager(this);
    BaseScene* newScenePointer = newScene.get();
    scene->second = std::move(newScene);
    SetScene(newScenePointer);
}

void SceneManager::ChangeState(std::unique_ptr<ISceneTransitionState> newState)
{
    state_ = std::move(newState);
}

void SceneManager::ChangeSceneActual()
{
    if (nextSceneName_)
    {
        if (isReloadRequested_)
        {
            ReloadScene(*nextSceneName_);
        }
        else if (BaseScene* scene = FindScene(*nextSceneName_))
        {
            SetScene(scene);
        }
        nextSceneName_ = std::nullopt;
        isReloadRequested_ = false;
    }
}

}