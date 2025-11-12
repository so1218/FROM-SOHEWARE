#pragma once

#include <cstdint>

// プレイヤー陣営
const uint32_t kCollisionAttributePlayer = 0b1;
const uint32_t kCollisionAttributePlayerWeaponKnife = 0b1 << 1;
const uint32_t kCollisionAttributePlayerWeaponAxe = 0b1 << 2;
// 敵陣営
const uint32_t kCollisionAttributeEnemy = 0b1 << 10;