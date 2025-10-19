#pragma once
#include "Collider.h"

#include <list>

class CollisionManager
{
public:

	void ClearColliders() { colliders_.clear(); }

	void AddCollider(Collider* collider);

	void CheckAllCollisions();

private:
	// コライダーリスト
	std::list<Collider*> colliders_;

	void CheckCollisionPair(Collider* colliderA, Collider* colliderB);
};