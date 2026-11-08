#pragma once
#include "Vector.h"

namespace FS
{

class GameObject;

// コライダーの形
enum class CollisionShapeType
{
	Sphere,   // 球
	Box,      // 箱。回転はせず、常にワールドの軸に沿う
};

// 判定に使うレイヤーの初期値。ゲーム側のレイヤーはこれ以外のビットを使う
constexpr uint32_t kDefaultCollisionLayer = 1u << 0;

// 当たり判定の形。GameObject::AddSphereCollider / AddBoxCollider で作る。
// 持ち主の GameObject が所有し、マネージャーへの登録と解除も自動で行う
class Collider
{
public:
	Collider(GameObject* owner, CollisionShapeType type);

	CollisionShapeType GetType() const { return type_; }

	// 球の半径。Sphere のときだけ使える
	void SetRadius(float radius, std::source_location location = std::source_location::current());
	float GetRadius() const { return radius_; }

	// 箱の大きさ (幅・高さ・奥行き)。Box のときだけ使える
	void SetSize(const Vector3& size, std::source_location location = std::source_location::current());
	const Vector3& GetSize() const { return size_; }
	Vector3 GetHalfSize() const { return size_ * 0.5f; }

	// 持ち主の位置から、コライダーの中心までのずれ
	void SetCenterOffset(const Vector3& offset) { centerOffset_ = offset; }
	const Vector3& GetCenterOffset() const { return centerOffset_; }

	// 中心のずれを持ち主の回転に合わせて回すか (初期値は回す)。箱そのものは回らない
	void SetRotateCenterOffset(bool isRotate) { isRotateCenterOffset_ = isRotate; }
	bool IsRotateCenterOffset() const { return isRotateCenterOffset_; }

	// 持ち主の位置 + 中心のずれ
	Vector3 GetWorldPosition() const;

	// 自分が属するレイヤーと、当たる相手のレイヤー (どちらもビットの組み合わせ)。
	// 2 つのコライダーは、お互いのマスクに相手のレイヤーが入っているときだけ判定する。
	// レイは、Raycast に渡したマスクにレイヤーが入っているコライダーに当たる
	void SetCollisionLayer(uint32_t layer) { collisionLayer_ = layer; }
	uint32_t GetCollisionLayer() const { return collisionLayer_; }
	void SetCollisionMask(uint32_t mask) { collisionMask_ = mask; }
	uint32_t GetCollisionMask() const { return collisionMask_; }

	// true にすると、ほかのコライダーとは判定せず、レイだけに当たる (弱点など)
	void SetRaycastOnly(bool isRaycastOnly) { isRaycastOnly_ = isRaycastOnly; }
	bool IsRaycastOnly() const { return isRaycastOnly_; }

	// 無効にすると、判定にもレイにも当たらない
	void SetEnable(bool isEnable) { isEnable_ = isEnable; }
	bool IsEnable() const { return isEnable_; }

	// デバッグ表示の色
	void SetColor(const Vector4& color) { color_ = color; }
	const Vector4& GetColor() const { return color_; }

	// 持ち主の GameObject
	GameObject* GetOwner() const { return owner_; }

	// 自分を基準にして、相手を押し戻すベクトルを計算
	bool CalculatePushBackVector(Collider* other, FS::Vector3& outPushVector) const;

private:
	GameObject* owner_ = nullptr;
	CollisionShapeType type_ = CollisionShapeType::Sphere;

	float radius_ = 0.5f;
	Vector3 size_ = { 1.0f, 1.0f, 1.0f };
	Vector3 centerOffset_ = { 0.0f, 0.0f, 0.0f };
	bool isRotateCenterOffset_ = true;

	uint32_t collisionLayer_ = kDefaultCollisionLayer;
	uint32_t collisionMask_ = kDefaultCollisionLayer;
	bool isRaycastOnly_ = false;
	bool isEnable_ = true;

	// デバッグ表示色
	Vector4 color_ = { 0.0f, 1.0f, 1.0f, 1.0f };
};

}
