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
    void Update();
    void Draw();
    void DebugDraw();

    // シーンを登録。シーン名で識別
    void RegisterScene(std::unique_ptr<BaseScene> scene);

    // 最初のシーンをフェードインでセット
    void SetInitialScene(const std::string& sceneName);

    // シーン切り替えをリクエスト
    void RequestSceneChange(const std::string& sceneName);

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
    BaseScene* FindScene(const std::string& sceneName) const;

    BaseScene* currentScene_ = nullptr;
    Engine* engine_ = nullptr;

    std::optional<std::string> nextSceneName_;
    std::map<std::string, std::unique_ptr<BaseScene>> scenes_;
    std::vector<std::string> sceneOrder_;   // 登録順

    std::unique_ptr<Fade> fade_;
    std::unique_ptr<ISceneTransitionState> state_;
};

}