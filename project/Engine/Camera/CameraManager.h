#pragma once
#include "Camera.h"
#include "ICameraController.h"
#include "ShakeEffect.h"

namespace FS
{

class CameraManager
{
public:
    CameraManager(Camera* camera);

    // コントローラーを切り替える
    void ChangeController(ICameraController* newController);

    // 更新処理
    void Update();

    // どこからでもカメラを揺らす
    void RequestShake(float duration, float intensity);
    void DebugDraw();
    void Draw();

    Camera* GetMainCamera() const { return mainCamera_; }

private:
    Camera* mainCamera_ = nullptr;
    ICameraController* currentController_ = nullptr;

    // 演出が終わった後に戻るための普段のカメラを記憶しておくポインタ
    ICameraController* defaultController_ = nullptr;

    ShakeEffect shake_;
};

}
