#pragma once
#include "GameObject.h"
#include "CameraPath.h"

namespace FE
{

class TrackCamera : public GameObject 
{
public:
    void Update() override;

    void DebugDraw() override;

private:
    CameraPath path_;
    Camera* camera_ = nullptr;
    float currentT_ = 0.0f;
    float moveSpeed_ = 0.1f; // 1秒間に進む割合(0~1)
};

}