#pragma once
#include <cstdint>

namespace FE::PIXColors
{
    // 0xFFRRGGBB 形式でカラーコードを定義
    constexpr uint32_t Geometry = 0xFFDC5050; // 赤系: 幾何描画
    constexpr uint32_t Shadow = 0xFF646464; // 灰系: 影生成
    constexpr uint32_t Compute = 0xFFFF8C00; // オレンジ系: CS 
    constexpr uint32_t PostProcess = 0xFFB450DC; // 紫系: ポストプロセス
    constexpr uint32_t UI = 0xFF50DC78; // 緑系: UI
}