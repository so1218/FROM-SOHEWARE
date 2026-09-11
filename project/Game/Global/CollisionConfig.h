#pragma once
#include <cstdint>

// 衝突属性定義
const uint32_t kCollisionAttributePlayer = 0b1;

const uint32_t kCollisionAttributeProp = 0b1 << 2;

const uint32_t kCollisionAttributeEnemy = 0b1 << 10;