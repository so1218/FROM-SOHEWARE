#pragma once

namespace FS
{

struct ProjectConfig
{
    std::wstring windowTitle = L"FROM SOHEWARE";
    // 窓で起動するときの大きさ (Windows の拡大率 100% のとき)。拡大率が 125% なら 1.25 倍の大きさになる
    int32_t width = 1280;
    int32_t height = 720;
    // true ならモニター全体を覆う枠なしのウィンドウで、モニターの解像度で描く
    bool isFullscreen = false;
    int32_t targetFPS = 60;
};

}