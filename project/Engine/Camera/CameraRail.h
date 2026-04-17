#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Engine.h"
#include "PropertyBinder.h"

namespace FE
{

class CameraManager;

struct CameraKeyframe
{
    Vector3 position;
    Quaternion rotation;
    Vector3 euler;
    float fov = 0.45f;
    float time = 1.0f;   // 次の点に到達するまでの時間
};

class CameraRail
{
public:
    CameraRail(Engine* engine, Camera* targetCamera, const std::string& railName);

    void Initialize();

    void AddKeyframe(const CameraKeyframe& keyframe);

    // 全体の進行時間を与えると、現在いるべき座標や回転を返す関数
    CameraKeyframe Evaluate(float currentTime) const;

    // レールの全長（秒数）
    float GetTotalTime() const;

    // エディター用のデバッグ線
    void DrawDebugSpline() const;

    bool DebugDraw();

private:
    Engine* engine_;
    Camera* targetCamera_;
    std::string railName_;
    std::unique_ptr<PropertyBinder> binder_;
    std::vector<CameraKeyframe> keyframes_;
    int32_t frameCount_ = 0;
};

}