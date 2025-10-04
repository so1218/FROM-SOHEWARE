#pragma once
#include "BaseScene.h"
#include "Engine.h"

class TitleScene : public BaseScene
{
public:
    TitleScene(Engine* engine, Camera* camera);

    // 初期化処理
    void Initialize() override;

    // 更新処理
    void Update() override;

    // 描画処理
    void Draw() override;

    // デバッグ描画処理
    void DebugDraw() override;

    // 終了処理
    void Finalize() override;

    // メンバー変数
    Engine* engine_;
    Camera* camera_;
};
