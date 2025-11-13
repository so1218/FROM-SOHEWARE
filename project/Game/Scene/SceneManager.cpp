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
}

void SceneManager::Update()
{
    // シーン切り替えリクエストがあれば実行
    if (nextSceneID_)
    {
        // マップから次のシーンを探す
        auto it = scenes_.find(*nextSceneID_);
        if (it != scenes_.end())
        {
            // 見つかったシーンをセットする
            SetScene(it->second.get());
        }
        // リクエストをクリア
        nextSceneID_ = std::nullopt;
    }

    // 現在のシーンを更新
    if (currentScene_)
    {
        currentScene_->Update();
    }
}

void SceneManager::Draw()
{
    if (currentScene_)
    {
        currentScene_->Draw();
    }
}

void SceneManager::DebugDraw()
{
    if (currentScene_)
    {
        currentScene_->DebugDraw();
    }
}

// シーンをマップに登録する
void SceneManager::RegisterScene(SceneID id, std::unique_ptr<BaseScene> scene)
{
    // SceneManagerをシーンに渡す
    scene->SetSceneManager(this);
    scenes_[id] = std::move(scene);
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