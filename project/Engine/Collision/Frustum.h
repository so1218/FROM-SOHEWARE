#pragma once
#include "Matrix.h"
#include "Vector.h"

namespace FE
{
class WorldTransform;

// 視界を構成する平面
struct Plane
{
    float a, b, c, d;
};

// 視界錐台 (Frustum)
struct Frustum
{
    Plane planes[6];

    // ビュープロジェクション行列からフラスタム（6つの平面）を抽出
    void ExtractFromMatrix(const Matrix4x4& vp)
    {
        // 左
        planes[0] = { vp.m[0][3] + vp.m[0][0], vp.m[1][3] + vp.m[1][0], vp.m[2][3] + vp.m[2][0], vp.m[3][3] + vp.m[3][0] };
        // 右
        planes[1] = { vp.m[0][3] - vp.m[0][0], vp.m[1][3] - vp.m[1][0], vp.m[2][3] - vp.m[2][0], vp.m[3][3] - vp.m[3][0] };
        // 下
        planes[2] = { vp.m[0][3] + vp.m[0][1], vp.m[1][3] + vp.m[1][1], vp.m[2][3] + vp.m[2][1], vp.m[3][3] + vp.m[3][1] };
        // 上
        planes[3] = { vp.m[0][3] - vp.m[0][1], vp.m[1][3] - vp.m[1][1], vp.m[2][3] - vp.m[2][1], vp.m[3][3] - vp.m[3][1] };
        // 近
        planes[4] = { vp.m[0][3] + vp.m[0][2], vp.m[1][3] + vp.m[1][2], vp.m[2][3] + vp.m[2][2], vp.m[3][3] + vp.m[3][2] };
        // 遠
        planes[5] = { vp.m[0][3] - vp.m[0][2], vp.m[1][3] - vp.m[1][2], vp.m[2][3] - vp.m[2][2], vp.m[3][3] - vp.m[3][2] };

        // 各平面の法線を正規化
        for (int i = 0; i < 6; ++i) 
        {
            float length = std::sqrt(planes[i].a * planes[i].a + planes[i].b * planes[i].b + planes[i].c * planes[i].c);
            planes[i].a /= length;
            planes[i].b /= length;
            planes[i].c /= length;
            planes[i].d /= length;
        }
    }

    // AABB（軸並行境界箱）が視界に入っているか判定
    bool IntersectsAABB(const Vector3& min, const Vector3& max) const
    {
        for (int i = 0; i < 6; ++i) 
        {
            // 平面の法線方向に最も近い点を求める
            Vector3 p = min;
            if (planes[i].a >= 0) p.x = max.x;
            if (planes[i].b >= 0) p.y = max.y;
            if (planes[i].c >= 0) p.z = max.z;

            // その点が平面の外側（裏側）にあるなら、AABB全体が視界外
            if (planes[i].a * p.x + planes[i].b * p.y + planes[i].c * p.z + planes[i].d < 0)
            {
                return false;
            }
        }
        return true; // どの平面の外側にも完全には出ていない＝視界内（または交差）
    }
};

}