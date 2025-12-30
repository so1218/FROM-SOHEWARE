#pragma once

#include <cstdint>

// 衝突属性ビット定義
const uint32_t kCollisionAttributePlayer = 0b1;
const uint32_t kCollisionAttributePlayerWeaponKnife = 0b1 << 1;
const uint32_t kCollisionAttributePlayerWeaponAxe = 0b1 << 2;

const uint32_t kCollisionAttributeEnemy = 0b1 << 10;

const uint32_t kCollisionAttributeExpGem = 0b1 << 20;