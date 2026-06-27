#pragma once
#include "Vector.h"

namespace FE
{

class GameObject;
class CollisionManager;

enum class CollisionShapeType
{
	Sphere, 
	AABB,   
};

class Collider
{
public:
	Collider(GameObject* owner);
	~Collider();

	// セッター
	void SetType(CollisionShapeType type) { type_ = type; }
	void SetRadius(float radius) { radius_ = radius; }
	void SetSize(const Vector3& size) { size_ = size; }
	void SetCollisionAttribute(uint32_t attribute) { collisionAttribute_ = attribute; }
	void SetCollisionMask(uint32_t mask) { collisionMask_ = mask; }
	void SetManager(CollisionManager* manager) { manager_ = manager; }
	// ゲッター
	CollisionShapeType GetType()const { return type_; }
	float GetRadius() const { return radius_; }
	Vector3 GetSize()const { return size_; }
	uint32_t GetCollisionAttribute() const { return collisionAttribute_; }
	uint32_t GetCollisionMask() const { return collisionMask_; }

	// 中心座標からのズレ
	void SetCenterOffset(const Vector3& offset) { centerOffset_ = offset; }

	// 親の座標＋オフセットを返す
	Vector3 GetWorldPosition() const;

	// 親オブジェクトを取得
	GameObject* GetOwner() const { return owner_; }

	// 衝突時コールバック関数
	void OnCollisionEnter(Collider* other);
	void OnCollisionStay(Collider* other);
	void OnCollisionExit(Collider* other);

	// デバッグ描画用カラー
	void SetColor(const Vector4& color) { color_ = color; }
	// コライダー描画（デバッグ用）
	void DrawCollider();

	void RegisterToManager();

private:
	GameObject* owner_ = nullptr; // 自分を持っている親
	Vector3 centerOffset_ = { 0.0f, 0.0f, 0.0f }; // ローカルオフセット

	// 自分を管理しているマネージャーのポインタ
	CollisionManager* manager_ = nullptr;

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
