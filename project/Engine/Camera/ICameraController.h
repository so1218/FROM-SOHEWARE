#pragma once
#include "Camera.h"

namespace FE
{

// カメラの基底クラス
class ICameraController
{
public:
    virtual ~ICameraController() = default;

    // 更新
    virtual void UpdateCamera(Camera* camera) = 0;

    // デバッグ表示
    virtual void DebugDraw() {}

    // 描画
    virtual void Draw() {}

    // カメラの状態をリセット（切り替え時やワープ時に使用）
    virtual void Reset(Camera* camera) {}

    // カメラシェイク
    virtual void StartShake(float duration, float intensity) {}
};

}