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

    // カメラを揺らす関数
    void RequestShake(float duration, float intensity);
    void DebugGui();
    void Draw();

    Camera* GetMainCamera() const { return mainCamera_; }

private:
    Camera* mainCamera_ = nullptr;
    ICameraController* currentController_ = nullptr;

    // 普段のカメラを記憶しておくポインタ
    ICameraController* defaultController_ = nullptr;

    ShakeEffect shake_;
};

}
