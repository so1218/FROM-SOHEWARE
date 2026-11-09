#pragma once
#include "Engine.h"
#include "ICameraController.h"
#include "PropertyBinder.h"

class FollowCamera;

// タイトル画面の固定カメラ
class TitleCamera : public FS::ICameraController
{
public:
    TitleCamera(FS::Engine* engine, FollowCamera* followCamera);

    void Initialize();
    void UpdateCamera(FS::Camera* camera) override;
    void DrawProperties();

    void StartTransition(FS::Camera* camera);
    // タイトルの位置に戻す
    void CancelTransition() { isTransitioning_ = false; }
    bool IsTransitionFinished() const { return isTransitioning_ && transitionTimer_ >= transitionTime_; }

private:
    void UpdateTitlePose(FS::Camera* camera);
    void UpdateTransition(FS::Camera* camera);

    FollowCamera* followCamera_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    float elapsedTime_ = 0.0f;
    bool isTransitioning_ = false;
    float transitionTimer_ = 0.0f;

    // 寄り始めたときのカメラ
    FS::Vector3 startPosition_;
    FS::Quaternion startRotation_;
    float startFov_ = 0.45f;

    // パラメータ
    FS::Vector3 position_ = { 0.0f, 20.0f, -60.0f };
    FS::Vector3 rotationDegrees_ = { 10.0f, 0.0f, 0.0f };
    float fov_ = 0.45f;
    float swayAmount_ = 0.3f;       
    float swaySpeed_ = 0.4f;
    float transitionTime_ = 2.5f;
    float arcHeight_ = 4.0f;        
};
