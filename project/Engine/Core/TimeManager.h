#pragma once

class TimeManager
{
public:
    static TimeManager* GetInstance();

    void Initialize();
    void Update();

    float GetDeltaTime() const { return deltaTime_ * timeScale_; }
    float GetUnscaledDeltaTime() const { return deltaTime_; }
    float GetTotalTime() const { return totalTime_; }
    float GetFPS() const { return fps_; }
    float GetAverageFPS() const { return averageFps_; }
    float GetTimeScale() const { return timeScale_; }

    void Pause() { isPaused_ = true; }
    void Resume() { isPaused_ = false; }
    bool IsPaused() const { return isPaused_; }
    void SetTimeScale(float scale) { timeScale_ = std::clamp(scale, 0.0f, 10.0f); };

    void Reset();

private:
    TimeManager() = default;
    ~TimeManager() = default;

    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;

    using Clock = std::chrono::steady_clock;
    Clock::time_point startTime_;
    Clock::time_point prevTime_;
    Clock::duration pausedDuration_ = Clock::duration::zero(); 
    Clock::time_point pauseStartTime_;

    float deltaTime_ = 0.0f;
    float totalTime_ = 0.0f;
    float fps_ = 0.0f;

    float timeScale_ = 1.0f;
    bool isPaused_ = false;

    // 平均FPS用
    float averageFps_ = 0.0f;
    int frameCount_ = 0;
    float timeElapsedForFps_ = 0.0f;
};