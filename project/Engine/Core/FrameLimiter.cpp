#include "FrameLimiter.h"

FrameLimiter::FrameLimiter(int targetFPS)
    : kTargetFPS_(targetFPS),
    kFrameDuration_(1000000 / targetFPS)
{
}

void FrameLimiter::Initialize()
{
    // システムタイマーの精度を上げる
    timeBeginPeriod(1); 
    targetTime_ = std::chrono::steady_clock::now();
}

void FrameLimiter::Finalize()
{
    // タイマー精度を元に戻す
    timeEndPeriod(1);
}

void FrameLimiter::WaitNextFrame()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - targetTime_);
    auto remaining = kFrameDuration_ - elapsed;

    if (remaining.count() > 2000)
    {
        // だいたいの時間だけスリープ
        std::this_thread::sleep_for(remaining - std::chrono::microseconds(2000));
    }

    // 調整
    while (std::chrono::steady_clock::now() - targetTime_ < kFrameDuration_)
    {}

    // 次のフレームの基準時間を更新
    targetTime_ = std::chrono::steady_clock::now();
}