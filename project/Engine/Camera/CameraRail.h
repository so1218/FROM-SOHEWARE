#pragma once
#include "Vector3.h"
#include "Quaternion.h"

namespace FE
{

struct CameraKeyframe
{
    Vector3 position;
    Quaternion rotation;
    float fov = 0.45f;
    float time = 1.0f;   // 次の点に到達するまでの時間
};

class CameraRail
{
public:
    void AddKeyframe(const CameraKeyframe& keyframe);

    // 全体の進行時間を与えると、現在いるべき座標や回転を返す関数
    CameraKeyframe Evaluate(float currentTime) const;

    // エディター用のデバッグ線
    void DrawDebugSpline() const;

private:
    std::vector<CameraKeyframe> keyframes_;
};

}