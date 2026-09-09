#include "pch.h"
#include "SceneManager.h"
#include "Engine.h"
#include "BaseScene.h"
#include "NormalState.h"
#include "FadeInState.h"
#include "ISceneTransitionState.h"
#include "Fade.h"

namespace FE
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
    if (currentScene_)
    {
        currentScene_->DebugDraw();
    }
	fade_->DebugDraw();
}

// シーンをマップに登録する
void SceneManager::RegisterScene(SceneID id, std::unique_ptr<BaseScene> scene)
{
    // SceneManagerをシーンに渡す
    scene->SetSceneManager(this);
    scenes_[id] = std::move(scene);
}

void SceneManager::SetInitialScene(SceneID initialSceneID)
{
    // マップから探す
    auto it = scenes_.find(initialSceneID);
    if (it != scenes_.end())
    {
        // SetSceneを直接呼んでシーンを初期化
        SetScene(it->second.get());

        // フェードイン状態へ切り替え
        ChangeState(std::make_unique<FadeInState>());
        fade_->Start(Fade::Status::FadeIn, fade_->GetDuration());
    }
}

// 切り替えたいシーンのIDをセットする
void SceneManager::RequestSceneChange(SceneID nextSceneID)
{
    nextSceneID_ = nextSceneID;
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

// 実際のシーン入れ替え処理
void SceneManager::ChangeSceneActual()
{
    if (nextSceneID_)
    {
        auto it = scenes_.find(*nextSceneID_);
        if (it != scenes_.end())
        {
            SetScene(it->second.get());
        }
        nextSceneID_ = std::nullopt;
    }
}

}