#pragma once
#include <cstdint>
#include <string>

struct ProjectConfig
{
    std::wstring windowTitle = L"FROM SOHEWARE";
    int32_t width = 1280;
    int32_t height = 720;
    int32_t targetFPS = 60;
};