#pragma once

#include "Vector.h"

enum class CollisionShapeType
{
	Sphere, 
	AABB,   
};

class Collider
{
public:
	Collider() = default;
	~Collider() = default;

	// セッター
	void SetType(CollisionShapeType type) { type_ = type; }
	void SetRadius(float radius) { radius_ = radius; }
	void SetSize(const Vector3& size) { size_ = size; }
	void SetCollisionAttribute(uint32_t attribute) { collisionAttribute_ = attribute; }
	void SetCollisionMask(uint32_t mask) { collisionMask_ = mask; }
	// ゲッター
	CollisionShapeType GetType()const { return type_; }
	float GetRadius() const { return radius_; }
	Vector3 GetSize()const { return size_; }
	uint32_t GetCollisionAttribute() const { return collisionAttribute_; }
	uint32_t GetCollisionMask() const { return collisionMask_; }

	virtual void OnCollision(Collider* other) = 0;
	// ワールド座標を取得
	virtual Vector3 GetWorldPosition() = 0;

	void SetColor(const Vector4& color) { color_ = color; }

	void DrawCollider();

private:
	// 形状タイプ
	CollisionShapeType type_ = CollisionShapeType::Sphere;

	// 衝突半径
	float radius_ = 1.0f;
	// AABB衝突ハーフサイズ
	Vector3 size_ = { 0.5f,0.5f,0.5f };
	// 衝突属性
	uint32_t collisionAttribute_ = 0xffffffff;
	// 衝突マスク(相手)
	uint32_t collisionMask_ = 0xffffffff;

	Vector4 color_ = { 0.0f, 1.0f, 0.0f, 1.0f };
};

