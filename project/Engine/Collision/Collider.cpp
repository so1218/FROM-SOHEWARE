#include "pch.h"
#include "Collider.h"
#include "DebugDraw.h"

// デバッグ用コライダー描画
void Collider::DrawCollider()
{
    Vector3 center = GetWorldPosition();

    // 球コライダー描画
    if (type_ == CollisionShapeType::Sphere)
    {
        DebugDraw::DrawSphere(center, radius_, color_);
    }
    // AABBコライダー描画
    else if (type_ == CollisionShapeType::AABB)
    {
        DebugDraw::DrawAABB(center - size_, center + size_, color_);
    }
}