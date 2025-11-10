#pragma once

#include <chrono>
#include <thread>
#include <Windows.h> 
#include <mmsystem.h>

class FrameLimiter
{
public:
    FrameLimiter(int targetFPS = 60);

    void Initialize();

    void Finalize();

    void WaitNextFrame();

private:
    const int kTargetFPS_;
    const std::chrono::microseconds kFrameDuration_;
    std::chrono::steady_clock::time_point targetTime_;
};