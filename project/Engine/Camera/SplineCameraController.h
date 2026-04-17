#pragma once
#include "ICameraController.h"
#include "CameraRail.h"

namespace FE
{

class SplineCameraController : public ICameraController
{
public:
    void Play(CameraRail* rail);
    void UpdateCamera(Camera* camera) override;
    void DebugDraw() override;
    void Draw() override;

    // 再生中かどうか
    bool IsPlaying() const { return isPlaying_; }

private:
    CameraRail* currentRail_ = nullptr;
    float currentPlayTime_ = 0.0f;
    bool isPlaying_ = false;
};

}