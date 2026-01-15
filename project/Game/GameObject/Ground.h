#pragma once
#include "Engine.h"

class Ground
{
public:
    // 初期化処理
    void Initialize(Engine* engine, Camera* camera);

    // 更新処理
    void Update();

    // 描画処理
    void Draw();

    // デバッグ描画処理
    void DebugDraw();

};

