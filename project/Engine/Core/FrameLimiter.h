#pragma once

class FrameLimiter
{
public:
    FrameLimiter(int targetFPS = 60);

    void Initialize();

    void Finalize();

    void WaitNextFrame();

private:
    const int targetFPS_;
    const std::chrono::microseconds frameDuration_;
    std::chrono::steady_clock::time_point targetTime_;
};