#pragma once

class BaseScene
{
public:
    virtual ~BaseScene() = default;

    virtual void Initialize() = 0;// 初期化
    virtual void Update() = 0;// 更新
    virtual void Draw() = 0;// 描画
	virtual void DebugDraw() = 0;// デバッグ描画
    virtual void Finalize() = 0;// 終了処理

    // SceneManager を注入
    virtual void SetSceneManager(class SceneManager* sceneManager) { sceneManager_ = sceneManager; }

protected:
    SceneManager* sceneManager_ = nullptr;
};

