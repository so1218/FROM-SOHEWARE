#include "SceneManager.h"
#include "GlobalVariables.h"

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
}

void SceneManager::Update()
{
    // フェードの状態によって処理を分岐
    switch (transitionState_)
    {
    case TransitionState::None:
        // 通常時
        // シーン切り替えリクエストがあれば、フェードアウト開始
        if (nextSceneID_)
        {
            transitionState_ = TransitionState::FadeOut;
            fade_->Start(Fade::Status::FadeOut, fade_->duration_);
        }
        else
        {
            if (currentScene_)
            {
                currentScene_->Update();
            }
            break;
        }

    case TransitionState::FadeOut:
        if (currentScene_)
        {
            currentScene_->Update();
        }

        fade_->Update(); 

        // フェードアウトが完了したら、実際のシーン切り替え処理
        if (fade_->IsFinished())
        {
            // マップから次のシーンを探す
            auto it = scenes_.find(*nextSceneID_);
            if (it != scenes_.end())
            {
                // 見つかったシーンをセットする
                SetScene(it->second.get());
            }

            nextSceneID_ = std::nullopt;

            // フェードインを開始
            transitionState_ = TransitionState::FadeIn;
            fade_->Start(Fade::Status::FadeIn, fade_->duration_);
        }
        break;

    case TransitionState::FadeIn:
        if (currentScene_)
        {
            currentScene_->Update(); 
        }
        fade_->Update(); 

        // フェードインが完了したら、通常状態に戻る
        if (fade_->IsFinished())
        {
            transitionState_ = TransitionState::None;
            fade_->Stop();
        }
        break;
    }
}

void SceneManager::Draw()
{
    if (currentScene_)
    {
        currentScene_->Draw();
    }

    fade_->Draw();
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

        // フェードインから開始するように状態をセット
        transitionState_ = TransitionState::FadeIn;
        fade_->Start(Fade::Status::FadeIn, fade_->duration_);
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

    engine_->particleSystem_->Clear();
    currentScene_ = newScene;

    // 新しいシーンの初期化処理
    if (currentScene_)
    {
        currentScene_->Initialize();
    }
}