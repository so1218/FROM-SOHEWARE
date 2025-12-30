#include "Collider.h"
#include "DebugDraw.h"

void Collider::DrawCollider()
{
	Vector3 center = GetWorldPosition();

	if (type_ == CollisionShapeType::Sphere) 
	{
		DebugDraw::DrawSphere(center, radius_, color_);
	}
	else if (type_ == CollisionShapeType::AABB)
	{
		DebugDraw::DrawAABB(center - size_, center + size_, color_);
	}
}