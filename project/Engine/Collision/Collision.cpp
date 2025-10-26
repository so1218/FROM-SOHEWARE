#include "Collision.h"
#include "Input.h"
#include "Engine.h"

bool IsCollision(const AABB& aabb1, const AABB& aabb2)
{
	if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
		(aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
		(aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z))
	{
		return true;
	}

	return false;
}

bool IsCollision(const Vector3& sphere1Pos, float sphere1Radius,
    const Vector3& sphere2Pos, float sphere2Radius) {
    // 2つの球の中心間の距離の2乗を計算
    float distanceSquared = Vector3::DistanceSquared(sphere1Pos, sphere2Pos);

    // 2つの球の半径の合計を計算し、その2乗を求める
    float sumOfRadii = sphere1Radius + sphere2Radius;
    float sumOfRadiiSquared = sumOfRadii * sumOfRadii;

    // 距離の2乗と半径の合計の2乗を比較して衝突を判定
    return distanceSquared < sumOfRadiiSquared;
}

Vector3 CalculatePenetrationVector(const AABB& a, const AABB& b)
{
    // 各軸の重なり距離を計算
    float dx1 = b.max.x - a.min.x; // bの右側とaの左側の距離
    float dx2 = a.max.x - b.min.x; // aの右側とbの左側の距離
    float overlapX = (dx1 < dx2) ? dx1 : -dx2;

    float dy1 = b.max.y - a.min.y;
    float dy2 = a.max.y - b.min.y;
    float overlapY = (dy1 < dy2) ? dy1 : -dy2;

    float dz1 = b.max.z - a.min.z;
    float dz2 = a.max.z - b.min.z;
    float overlapZ = (dz1 < dz2) ? dz1 : -dz2;

    // 最小の重なり軸を選ぶ（押し戻し方向）
    float absX = std::abs(overlapX);
    float absY = std::abs(overlapY);
    float absZ = std::abs(overlapZ);

    if (absX < absY && absX < absZ) {
        return Vector3{ overlapX, 0.0f, 0.0f };
    }
    else if (absY < absZ) {
        return Vector3{ 0.0f, overlapY, 0.0f };
    }
    else {
        return Vector3{ 0.0f, 0.0f, overlapZ };
    }
}

bool IsMouseHitObject(const Vector3& objectWorldPos, float radius, const Matrix4x4& viewProjection) 
{
    // オブジェクトのワールド位置をクリップ座標に変換
    Vector4 worldPos = { objectWorldPos.x, objectWorldPos.y, objectWorldPos.z, 1.0f };
    Vector4 clipPos = viewProjection * worldPos;

    // w除算（NDC）
    if (clipPos.w == 0.0f) return false;
    Vector3 ndcPos = 
    {
        clipPos.x / clipPos.w,
        clipPos.y / clipPos.w,
        clipPos.z / clipPos.w
    };

    // NDC → スクリーン座標へ変換
    float screenX = (ndcPos.x + 1.0f) * 0.5f * kClientWidth;
    float screenY = (1.0f - ndcPos.y) * 0.5f * kClientHeight;

    // マウス位置取得
    Vector2 mousePos = Input::GetInstance().GetMousePosition();

    // マウス位置とオブジェクト位置（スクリーン座標）で距離を取って比較
    float dx = mousePos.x - screenX;
    float dy = mousePos.y - screenY;
    float distanceSq = dx * dx + dy * dy;

    return distanceSq <= radius * radius; // 半径内ならヒット
}