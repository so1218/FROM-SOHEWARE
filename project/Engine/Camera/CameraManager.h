#pragma once
#include "Camera.h"
#include "ICameraController.h"
#include "CameraRail.h"
#include "SplineCameraController.h"
#include "ShakeEffect.h"

namespace FE
{

class CameraManager
{
public:
    CameraManager(Camera* camera);

    // コントローラーを切り替える
    void ChangeController(ICameraController* newController);

    // レールの登録
    void AddRail(const std::string& name, std::unique_ptr<CameraRail> rail);

    void PlayRail(const std::string& name);

    // 更新処理
    void Update();

    // どこからでもカメラを揺らす
    void RequestShake(float duration, float intensity);
    void DebugDraw();
    void Draw();

    Camera* GetMainCamera() const { return mainCamera_; }

    CameraRail* GetRail(const std::string& name);

private:
    Camera* mainCamera_ = nullptr;
    ICameraController* currentController_ = nullptr;

    // 演出が終わった後に戻るための普段のカメラを記憶しておくポインタ
    ICameraController* defaultController_ = nullptr;

    // レール機能の管理
    std::unordered_map<std::string, std::unique_ptr<CameraRail>> rails_;
    std::unique_ptr<SplineCameraController> splineController_;

    ShakeEffect shake_;
};

}
