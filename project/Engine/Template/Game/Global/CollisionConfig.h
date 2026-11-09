#pragma once
#include <cstdint>

// コライダーのレイヤー。1u << 0 はエンジンの既定のレイヤーなので、1u << 1 から使う
namespace CollisionLayer
{
    constexpr uint32_t Player = 1u << 1;
    constexpr uint32_t Enemy = 1u << 2;
}
