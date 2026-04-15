#include "pch.h"
#include "CameraRail.h"
#include "DebugDraw.h"

namespace FE
{

void CameraRail::AddKeyframe(const CameraKeyframe& keyframe)
{
    keyframes_.push_back(keyframe);
}

CameraKeyframe CameraRail::Evaluate(float currentTime) const
{
    // キーフレームが無い、または1つしかない場合の安全対策
    if (keyframes_.empty()) return CameraKeyframe{};
    if (keyframes_.size() == 1) return keyframes_[0];

    // 現在、どのインデックスにいるのかを計算
    float localTime = currentTime;
    size_t p1Index = 0;

    for (size_t i = 0; i < keyframes_.size() - 1; ++i)
    {
        if (localTime <= keyframes_[i].time)
        {
            p1Index = i;
            break;
        }
        // 時間がオーバーしていたら、その区間の時間を引いて次の区間へ
        localTime -= keyframes_[i].time;
        p1Index = i + 1;
    }

    // もし指定時間がレールの総時間を超えていたら、最後のキーフレームを返す
    if (p1Index >= keyframes_.size() - 1)
    {
        return keyframes_.back();
    }

    // その区間の中での進行度tを計算
    float segmentDuration = keyframes_[p1Index].time;
    float t = (segmentDuration > 0.0f) ? (localTime / segmentDuration) : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    // Catmull-Rom補間に必要な4つの点を用意
    size_t p0Index = (p1Index > 0) ? p1Index - 1 : p1Index;
    size_t p2Index = p1Index + 1;
    size_t p3Index = (p2Index + 1 < keyframes_.size()) ? p2Index + 1 : p2Index;

    const CameraKeyframe& p0 = keyframes_[p0Index];
    const CameraKeyframe& p1 = keyframes_[p1Index];
    const CameraKeyframe& p2 = keyframes_[p2Index];
    const CameraKeyframe& p3 = keyframes_[p3Index];

    CameraKeyframe result;

    // Catmull-Romスプライン曲線の計算
    float t2 = t * t;
    float t3 = t2 * t;

    result.position = (
        (p1.position * 2.0f) +
        (p0.position * -1.0f + p2.position) * t +
        (p0.position * 2.0f - p1.position * 5.0f + p2.position * 4.0f - p3.position) * t2 +
        (p0.position * -1.0f + p1.position * 3.0f - p2.position * 3.0f + p3.position) * t3
        ) * 0.5f;

    // 球面線形補間 
    result.rotation = Quaternion::Slerp(p1.rotation, p2.rotation, t);

    // FOV:線形補間
    result.fov = p1.fov + (p2.fov - p1.fov) * t;

    result.time = 0.0f;

    return result;
}

float CameraRail::GetTotalTime() const 
{
    float total = 0.0f;
    if (keyframes_.empty()) return 0.0f;
    // 最後のキーフレームを除いた時間の合計
    for (size_t i = 0; i < keyframes_.size() - 1; ++i) 
    {
        total += keyframes_[i].time;
    }
    return total;
}

void CameraRail::DrawDebugSpline() const
{
    if (keyframes_.size() < 2) return;

    // レールの総時間を計算
    float totalTime = 0.0f;
    for (size_t i = 0; i < keyframes_.size() - 1; ++i)
    {
        totalTime += keyframes_[i].time;
    }

    // 色の定義
    Vector4 lineColor = { 0.0f, 1.0f, 0.0f, 1.0f };  
    Vector4 pointColor = { 1.0f, 0.0f, 0.0f, 1.0f }; 

    // 0.1秒刻みでEvaluateを呼び、点と点を線で結ぶ
    const float step = 0.1f;
    Vector3 prevPos = Evaluate(0.0f).position;

    for (float t = step; t <= totalTime; t += step)
    {
        Vector3 currentPos = Evaluate(t).position;

        // 線を描画
        DebugDraw::DrawLine(prevPos, currentPos, lineColor);

        prevPos = currentPos;
    }

    // ループの端数のズレを防ぐため、最後の隙間をきっちり結ぶ
    Vector3 lastPos = Evaluate(totalTime).position;
    DebugDraw::DrawLine(prevPos, lastPos, lineColor);

    // キーフレームの点自体も描画
    for (const auto& kf : keyframes_)
    {
        // コントロールポイントの位置を視覚化
        DebugDraw::DrawSphere(kf.position, 0.5f, pointColor);
    }
}

}