#pragma once
#include <memory> 
#include <map>
#include <optional>

#include "Fade.h"
#include "BaseScene.h"
#include "Engine.h"

class ISceneTransitionState;

// シーンを識別するためのID
enum class SceneID
{
    Title,
    Play,
    TestHori,
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

    SceneManager();

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

    // 状態クラスからアクセスするためのゲッター
    Fade* GetFade() const { return fade_.get(); }
    BaseScene* GetCurrentScene() const { return currentScene_; }

    // 次のシーン予約があるか確認
    bool HasNextSceneID() const { return nextSceneID_.has_value(); }

    // 状態を切り替える関数
    void ChangeState(std::unique_ptr<ISceneTransitionState> newState);
    // 実際のシーン入れ替え処理
    void ChangeSceneActual();

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

    std::unique_ptr<ISceneTransitionState> state_;
};

