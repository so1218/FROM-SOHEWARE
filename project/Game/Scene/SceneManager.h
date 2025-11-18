#pragma once
#include <memory> 
#include <map>
#include <optional>

#include "Fade.h"
#include "BaseScene.h"
#include "Engine.h"

// シーンを識別するためのID
enum class SceneID
{
    Title,
    Play,
    Sample,
};

class SceneManager
{
public:
    enum class TransitionState
    {
        None,     // 通常時
        FadeOut,  // フェードアウト
        FadeIn    // フェードイン
    };

    SceneManager() : currentScene_(nullptr) {}

    ~SceneManager();

    void Initialize(Engine* engine);

    // 現在のシーンを更新
    void Update();

    // 現在のシーンを描画
    void Draw();

	// 現在のシーンのデバッグ描画
    void DebugDraw();

    // シーンを登録するための関数
    void RegisterScene(SceneID id, std::unique_ptr<BaseScene> scene);

    // 最初のシーンをフェードインでセット
    void SetInitialScene(SceneID initialSceneID);

    // IDでシーン切り替えをリクエストする関数
    void RequestSceneChange(SceneID nextSceneID);

private:
    // シーンを設定する
    void SetScene(BaseScene* newScene);

    // 現在のシーン
    BaseScene* currentScene_ = nullptr;

	Engine* engine_ = nullptr;

    // 次に切り替えるシーンのIDを保持する
    std::optional<SceneID> nextSceneID_ = std::nullopt;

    // すべてのシーンを保持するマップ
    std::map<SceneID, std::unique_ptr<BaseScene>> scenes_;

    std::unique_ptr<Fade> fade_; 
    TransitionState transitionState_ = TransitionState::None;
};

