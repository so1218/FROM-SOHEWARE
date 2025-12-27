#pragma once
#include "Structures.h"

bool IsCollision(const AABB& aabb1, const AABB& aabb2);
bool IsCollision(const Vector3& sphere1Pos, float sphere1Radius,
    const Vector3& sphere2Pos, float sphere2Radius);
bool IsCollision(const AABB& aabb, const Vector3& spherePos, float sphereRadius);

// 重なりの分だけ押し戻すベクトルを計算
Vector3 CalculatePenetrationVector(const AABB& a, const AABB& b);

bool IsMouseHitObject(const Vector3& objectWorldPos, float radius, const Matrix4x4& viewProjection);