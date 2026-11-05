#pragma once

namespace FS
{

struct ProjectConfig
{
    // ゲーム側で決める。何も決めなければエンジンの名前
    std::wstring windowTitle = L"FromSoheWare";
    // 窓で起動するときの大きさ (Windows の拡大率 100% のとき)。拡大率が 125% なら 1.25 倍の大きさ
    int32_t width = 1280;
    int32_t height = 720;
    // true なら枠なしのフルスクリーンで、モニターの解像度
    bool isFullscreen = false;
    int32_t targetFPS = 60;
};

}