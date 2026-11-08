#pragma once
#include <cstdint>

// コライダーのレイヤー
namespace CollisionLayer
{
    constexpr uint32_t Player = 1u << 1;
    constexpr uint32_t Enemy = 1u << 2;
    constexpr uint32_t Prop = 1u << 3;
}
