#pragma once
#include "Vector3.h"

class ShakeEffect
{
public:
    void Start(float duration, float intensity);
    void Update();

    // 現在のシェイクオフセットを返す
    Vector3 GetOffset() const;

    bool IsActive() const { return isActive_; }

    bool IsJustFinished() const { return (wasActive_ && !isActive_); };

private:
    float duration_ = 0.0f;
    float timer_ = 0.0f;
    float intensity_ = 0.0f;
    bool isActive_ = false;
    bool wasActive_ = false;
};