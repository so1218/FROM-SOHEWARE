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

    void Draw() override
    {
        if (currentRail_) currentRail_->DrawDebugSpline();
    }

private:
    CameraRail* currentRail_ = nullptr;
    float currentPlayTime_ = 0.0f;
    bool isPlaying_ = false;
};

}