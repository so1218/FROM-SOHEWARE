#pragma once
#include <memory> 

#include "BaseScene.h"

class SceneManager
{
public:
    SceneManager() : currentScene_(nullptr) {}

    // シーンを設定する
    void SetScene(std::unique_ptr<BaseScene> newScene);

    // 現在のシーンを更新
    void Update();

    // 現在のシーンを描画
    void Draw();

    // 現在のシーンを初期化
    void Initialize();

    // 現在のシーンを終了
    void Finalize();

    void RequestSceneChange(std::unique_ptr<BaseScene> newScene) { nextScene_ = std::move(newScene); }
   
private:
    std::unique_ptr<BaseScene> currentScene_;
    std::unique_ptr<BaseScene> nextScene_;
};

