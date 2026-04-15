#pragma once
#include "Camera.h"
#include "ICameraController.h"

namespace FE
{

class CameraManager
{
public:
    CameraManager(Camera* mainCamera) : mainCamera_(mainCamera) {}

    // コントローラーを切り替える
    void ChangeController(ICameraController* newController);

    // 更新処理
    void Update();

    // どこからでもカメラを揺らす
    void RequestShake(float duration, float intensity);
    void DebugDraw();
    void Draw();

    // 現在のコントローラーを取得（デバッグ描画などで使う用）
    ICameraController* GetActiveController() const { return activeController_; }

private:
    Camera* mainCamera_ = nullptr;
    ICameraController* activeController_ = nullptr;
};

}
