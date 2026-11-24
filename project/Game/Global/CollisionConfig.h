#pragma once
#include <cstdint>

// コライダーのレイヤー
namespace CollisionLayer
{
    constexpr uint32_t Player = 1u << 1;
    constexpr uint32_t Enemy = 1u << 2;
    constexpr uint32_t Prop = 1u << 3;
    // 敵の弱点 (ゾンビの頭など)。体の当たり判定の中にあっても、弾はこちらを優先する
    constexpr uint32_t EnemyWeakPoint = 1u << 4;
}
