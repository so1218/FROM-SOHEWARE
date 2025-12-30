#include "Collision.h"
#include "Input.h"
#include "Engine.h"

// AABB同士の衝突判定
bool IsCollision(const AABB& aabb1, const AABB& aabb2)
{
    if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
        (aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
        (aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z))
    {
        return true;
    }
    return false;
}

// 球同士の衝突判定
bool IsCollision(const Vector3& sphere1Pos, float sphere1Radius,
    const Vector3& sphere2Pos, float sphere2Radius)
{
    // 中心間距離の2乗
    float distanceSquared = Vector3::DistanceSquared(sphere1Pos, sphere2Pos);

    // 半径和の2乗
    float sumOfRadii = sphere1Radius + sphere2Radius;
    float sumOfRadiiSquared = sumOfRadii * sumOfRadii;

    return distanceSquared < sumOfRadiiSquared;
}

// AABBと球の衝突判定
bool IsCollision(const AABB& aabb, const Vector3& spherePos, float sphereRadius)
{
    // 球中心に最も近いAABB上の点を取得
    float closestX = std::clamp(spherePos.x, aabb.min.x, aabb.max.x);
    float closestY = std::clamp(spherePos.y, aabb.min.y, aabb.max.y);
    float closestZ = std::clamp(spherePos.z, aabb.min.z, aabb.max.z);

    Vector3 closestPoint = { closestX, closestY, closestZ };

    // 最近点との距離の2乗
    float distanceSquared =
        (closestPoint.x - spherePos.x) * (closestPoint.x - spherePos.x) +
        (closestPoint.y - spherePos.y) * (closestPoint.y - spherePos.y) +
        (closestPoint.z - spherePos.z) * (closestPoint.z - spherePos.z);

    return distanceSquared <= sphereRadius * sphereRadius;
}

// AABB同士の押し戻しベクトルを計算（最小重なり軸）
Vector3 CalculatePenetrationVector(const AABB& a, const AABB& b)
{
    // 各軸の重なり量
    float dx1 = b.max.x - a.min.x;
    float dx2 = a.max.x - b.min.x;
    float overlapX = (dx1 < dx2) ? dx1 : -dx2;

    float dy1 = b.max.y - a.min.y;
    float dy2 = a.max.y - b.min.y;
    float overlapY = (dy1 < dy2) ? dy1 : -dy2;

    float dz1 = b.max.z - a.min.z;
    float dz2 = a.max.z - b.min.z;
    float overlapZ = (dz1 < dz2) ? dz1 : -dz2;

    // 最も小さい重なり方向を返す
    float absX = std::abs(overlapX);
    float absY = std::abs(overlapY);
    float absZ = std::abs(overlapZ);

    if (absX < absY && absX < absZ)
    {
        return Vector3{ overlapX, 0.0f, 0.0f };
    }
    else if (absY < absZ)
    {
        return Vector3{ 0.0f, overlapY, 0.0f };
    }
    else
    {
        return Vector3{ 0.0f, 0.0f, overlapZ };
    }
}

// マウスがオブジェクトにヒットしているか
bool IsMouseHitObject(const Vector3& objectWorldPos, float radius,
    const Matrix4x4& viewProjection)
{
    // ワールド→クリップ座標
    Vector4 worldPos = { objectWorldPos.x, objectWorldPos.y, objectWorldPos.z, 1.0f };
    Vector4 clipPos = viewProjection * worldPos;

    // NDC
    if (clipPos.w == 0.0f) return false;
    Vector3 ndcPos =
    {
        clipPos.x / clipPos.w,
        clipPos.y / clipPos.w,
        clipPos.z / clipPos.w
    };

    // スクリーン座標
    float screenX = (ndcPos.x + 1.0f) * 0.5f * kClientWidth;
    float screenY = (1.0f - ndcPos.y) * 0.5f * kClientHeight;

    // マウスとの距離判定
    Vector2 mousePos = Input::GetInstance().GetMousePosition();
    float dx = mousePos.x - screenX;
    float dy = mousePos.y - screenY;

    return (dx * dx + dy * dy) <= radius * radius;
}