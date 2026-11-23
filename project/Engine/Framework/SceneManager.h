#pragma once

namespace FS
{
class Engine;
class BaseScene;
class ISceneTransitionState;
class Fade;

class SceneManager
{
public:
    SceneManager();
    ~SceneManager();

    void Initialize(Engine* engine);
    void Finalize();
    void Update();
    void Draw();
    void DebugGui();

    // シーンを作る関数を登録し、1 つ作っておく
    void RegisterScene(std::function<std::unique_ptr<BaseScene>()> factory);
    // 作り直さないシーンを登録
    void RegisterScene(std::unique_ptr<BaseScene> scene);

    // 最初のシーンをフェードインでセット
    void SetInitialScene(const std::string& sceneName);

    // シーン切り替えをリクエスト
    void RequestSceneChange(const std::string& sceneName);
    // 今のシーンを捨てて作り直す
    void RequestSceneReload();

    // 登録順のシーン名一覧 (デバッグ UI 用)
    const std::vector<std::string>& GetSceneNames() const { return sceneOrder_; }

    // 状態クラスからアクセスするためのもの
    Fade* GetFade() const { return fade_.get(); }
    BaseScene* GetCurrentScene() const { return currentScene_; }
    bool HasNextScene() const { return nextSceneName_.has_value(); }
    void ChangeState(std::unique_ptr<ISceneTransitionState> newState);
    void ChangeSceneActual();

private:
    void SetScene(BaseScene* newScene);
    // 古いシーンを終わらせてから捨て場へ移し、新しく作ったシーンに切り替える
    void ReloadScene(const std::string& sceneName);
    BaseScene* FindScene(const std::string& sceneName) const;

    BaseScene* currentScene_ = nullptr;
    Engine* engine_ = nullptr;

    std::optional<std::string> nextSceneName_;
    bool isReloadRequested_ = false;
    std::map<std::string, std::unique_ptr<BaseScene>> scenes_;
    std::map<std::string, std::function<std::unique_ptr<BaseScene>()>> sceneFactories_;
    // 作り直す前のシーン
    std::unique_ptr<BaseScene> retiredScene_;
    std::vector<std::string> sceneOrder_;   // 登録順

    std::unique_ptr<Fade> fade_;
    std::unique_ptr<ISceneTransitionState> state_;
};

}