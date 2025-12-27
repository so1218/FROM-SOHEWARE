#include "Collider.h"
#include "DebugDraw.h"

void Collider::DrawCollider()
{
	Vector3 center = GetWorldPosition();
	Vector4 color = { 0.0f, 1.0f, 0.0f, 1.0f };

	if (type_ == CollisionShapeType::Sphere) 
	{
		DebugDraw::DrawSphere(center, radius_, color);
	}
	else if (type_ == CollisionShapeType::AABB)
	{
		DebugDraw::DrawAABB(center - size_, center + size_, color);
	}
}