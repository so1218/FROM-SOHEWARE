#pragma once
#include <memory> 
#include <map>
#include <optional>

#include "BaseScene.h"

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
    SceneManager() : currentScene_(nullptr) {}

    ~SceneManager();

    // 現在のシーンを更新
    void Update();

    // 現在のシーンを描画
    void Draw();

    // シーンを登録するための関数
    void RegisterScene(SceneID id, std::unique_ptr<BaseScene> scene);

    // IDでシーン切り替えをリクエストする関数
    void RequestSceneChange(SceneID nextSceneID);

private:
    // シーンを設定する
    void SetScene(BaseScene* newScene);

    // 現在のシーン
    BaseScene* currentScene_ = nullptr;

    // 次に切り替えるシーンのIDを保持する
    std::optional<SceneID> nextSceneID_ = std::nullopt;

    // すべてのシーンを保持するマップ
    std::map<SceneID, std::unique_ptr<BaseScene>> scenes_;
};

