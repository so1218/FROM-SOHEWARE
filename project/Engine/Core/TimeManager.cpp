#include "TimeManager.h"
#include <Windows.h>

TimeManager* TimeManager::GetInstance()
{
    static TimeManager instance;
    return &instance;
}

void TimeManager::Initialize()
{
    startTime_ = Clock::now();
    prevTime_ = startTime_;
    pausedDuration_ = Clock::duration::zero();
    deltaTime_ = 0.0f;
    totalTime_ = 0.0f;
    fps_ = 0.0f;
    isPaused_ = false;
    averageFps_ = 0.0f;
    frameCount_ = 0;
    timeElapsedForFps_ = 0.0f;
}

void TimeManager::Update()
{
    auto currentTime = Clock::now();

    if (isPaused_)
    {
        // ポーズ開始時刻を記録（初回のみ）
        if (pauseStartTime_ == Clock::time_point{})
            pauseStartTime_ = currentTime;

        deltaTime_ = 0.0f;
        fps_ = 0.0f;

        return;
    }
    else
    {
        // ポーズ解除直後にポーズ時間を加算
        if (pauseStartTime_ != Clock::time_point{})
        {
            pausedDuration_ += currentTime - pauseStartTime_;
            pauseStartTime_ = Clock::time_point{};

            prevTime_ = currentTime;
        }
    }

    std::chrono::duration<float> frameDuration = currentTime - prevTime_;
    std::chrono::duration<float> totalDuration = currentTime - startTime_ - pausedDuration_;

    deltaTime_ = frameDuration.count();
    totalTime_ = totalDuration.count();

    if (deltaTime_ > 0.0001f) 
    {
        fps_ = 1.0f / deltaTime_;
    }
    else
    {
        fps_ = 0.0f;
    }

    // 平均FPSの計算

    // 1フレーム分の時間とカウントを加算
    timeElapsedForFps_ += deltaTime_;
    frameCount_++;

    // 経過時間が1秒を超えたら平均FPSを計算
    if (timeElapsedForFps_ >= 1.0f)
    {
        // 1秒間に描画したフレーム数
        averageFps_ = static_cast<float>(frameCount_) / timeElapsedForFps_;

        // 次の1秒間のためにリセット
        frameCount_ = 0;
        timeElapsedForFps_ -= 1.0f;
    }

    prevTime_ = currentTime;
}

void TimeManager::Reset()
{
    auto currentTime = Clock::now();
    startTime_ = currentTime;
    prevTime_ = currentTime;
    pausedDuration_ = Clock::duration::zero();
    pauseStartTime_ = Clock::time_point{};
    deltaTime_ = 0.0f;
    totalTime_ = 0.0f;
    fps_ = 0.0f;
    isPaused_ = false;
    averageFps_ = 0.0f;
    frameCount_ = 0;
    timeElapsedForFps_ = 0.0f;
}