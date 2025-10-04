#pragma once
#include <chrono>

class TimeManager
{
public:
    static TimeManager* GetInstance();

    void Initialize();
    void Update();

    float GetDeltaTime() const { return deltaTime_; }
    float GetTotalTime() const { return totalTime_; }
    float GetFPS() const { return fps_; }

    void Pause() { isPaused_ = true; }
    void Resume() { isPaused_ = false; }
    bool IsPaused() const { return isPaused_; }

    void Reset();

private:
    TimeManager() = default;
    ~TimeManager() = default;

    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;

    using Clock = std::chrono::steady_clock;
    Clock::time_point startTime_;
    Clock::time_point prevTime_;
    Clock::duration pausedDuration_ = Clock::duration::zero();  // ポーズ中の経過時間を保持
    Clock::time_point pauseStartTime_;

    float deltaTime_ = 0.0f;
    float totalTime_ = 0.0f;
    float fps_ = 0.0f;

    bool isPaused_ = false;
};