#pragma once
#include "Vector.h"

namespace FE
{

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

	// 衝突時コールバック
	virtual void OnCollisionEnter(Collider* other) {}
	virtual void OnCollisionStay(Collider* other) {}
	virtual void OnCollisionExit(Collider* other) {}
	// ワールド座標取得
	virtual Vector3 GetWorldPosition() const = 0;
	// デバッグ描画用カラー
	void SetColor(const Vector4& color) { color_ = color; }
	// コライダー描画（デバッグ用）
	void DrawCollider();

private:
	// 形状タイプ
	CollisionShapeType type_ = CollisionShapeType::Sphere;

	// 球コライダー半径
	float radius_ = 1.0f;
	// AABBハーフサイズ
	Vector3 size_ = { 0.5f,0.5f,0.5f };
	// 衝突属性
	uint32_t collisionAttribute_ = 0xffffffff;
	// 衝突マスク(相手)
	uint32_t collisionMask_ = 0xffffffff;
	// デバッグ表示色
	Vector4 color_ = { 0.0f, 1.0f, 1.0f, 1.0f };
};

}
