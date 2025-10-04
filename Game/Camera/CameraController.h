#pragma once
#include "Engine.h"

class Player;

// 矩形
struct Rect
{
	float left = 0.0f;
	float right = 1.0f;
	float bottom = 0.0f;
	float top = 1.0f;
};

class CameraController
{
public:
	void Initialize(Camera* camera);

	void Update();

	void Reset();

	void SetTarget(Player* target) { target_ = target; };

	void SetMovableArea(Rect area) { movableArea_ = area; }

private:
	// ビュープロジェクション
	Matrix4x4 viewProjectionMatrix;

	// カメラ
	Camera* camera_ = nullptr;

	Player* target_ = nullptr;

	// 追従対象とカメラの座標の差(オフセット)
	Vector3 targetoffset_ = { 0, 5, -50.0f };

	// カメラの移動範囲
	Rect movableArea_ = { 0, 200, 0, 200 };
	// カメラの目標座標
	Vector3 targetPosition;

	// 座標補間割合
	static inline const float kInterpolationRate = 0.6f;
	// 速度掛け率
	static inline const float kVelocityBias = 10.0f;
	// 追従対象の各方向へのカメラ移動範囲
	static inline const Rect margin = { 0, 100, 0, 100 };
};

