#pragma once
#include "Structures.h"

// AABB同士の衝突判定
bool IsCollision(const AABB& aabb1, const AABB& aabb2);

// 球同士の衝突判定
bool IsCollision(const Vector3& sphere1Pos, float sphere1Radius,
    const Vector3& sphere2Pos, float sphere2Radius);

// AABBと球の衝突判定
bool IsCollision(const AABB& aabb, const Vector3& spherePos, float sphereRadius);

// AABB同士の押し戻しベクトルを計算
Vector3 CalculatePenetrationVector(const AABB& a, const AABB& b);

// マウスがオブジェクトにヒットしているか
bool IsMouseHitObject(const Vector3& objectWorldPos, float radius,
    const Matrix4x4& viewProjection);