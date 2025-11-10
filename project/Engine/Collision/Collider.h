#pragma once

#include "Vector.h"

class Collider
{
public:
	// コンストラクタ
	Collider() = default;
	// デストラクタ
	~Collider() = default;
	// 衝突半径を設定
	void SetRadius(float radius) { radius_ = radius; }
	void SetCollisionAttribute(uint32_t attribute) { collisionAttribute_ = attribute; }
	void SetCollisionMask(uint32_t mask) { collisionMask_ = mask; }
	// 衝突半径を取得
	float GetRadius() const { return radius_; }
	uint32_t GetCollisionAttribute() const { return collisionAttribute_; }
	uint32_t GetCollisionMask() const { return collisionMask_; }
	// 衝突判定のための純粋仮想関数
	virtual void OnCollision(Collider* other) = 0;

	// ワールド座標を取得
	virtual Vector3 GetWorldPosition() = 0;

private:
	// 衝突半径
	float radius_ = 1.0f;
	// 衝突属性
	uint32_t collisionAttribute_ = 0xffffffff;
	// 衝突マスク(相手)
	uint32_t collisionMask_ = 0xffffffff;
};

