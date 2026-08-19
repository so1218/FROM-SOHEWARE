#include "pch.h"
#include "DebugDraw.h"

#ifdef ENABLE_DEBUG_DRAW

#include "RendererManager.h"
#include "Camera.h"
#include "MathUtils.h" 

namespace FE
{

RendererManager* DebugDraw::rendererManager_ = nullptr;

void DebugDraw::Initialize(RendererManager* rendererManager)
{
    rendererManager_ = rendererManager;
}

void DebugDraw::DrawLine(const Vector3& start, const Vector3& end, const Vector4& color)
{
    if (!rendererManager_) return;

    uint32_t colorU = Math::ColorVectorToUint32(color); 
    rendererManager_->SubmitLine(start, end, colorU);
}

void DebugDraw::DrawAABB(const Vector3& min, const Vector3& max, const Vector4& color)
{
    // 下面
    Vector3 p0 = { min.x, min.y, min.z };
    Vector3 p1 = { max.x, min.y, min.z };
    Vector3 p2 = { max.x, min.y, max.z };
    Vector3 p3 = { min.x, min.y, max.z };

    // 上面
    Vector3 p4 = { min.x, max.y, min.z };
    Vector3 p5 = { max.x, max.y, min.z };
    Vector3 p6 = { max.x, max.y, max.z };
    Vector3 p7 = { min.x, max.y, max.z };

    // 下面の枠
    DrawLine(p0, p1, color);
    DrawLine(p1, p2, color);
    DrawLine(p2, p3, color);
    DrawLine(p3, p0, color);

    // 上面の枠
    DrawLine(p4, p5, color);
    DrawLine(p5, p6, color);
    DrawLine(p6, p7, color);
    DrawLine(p7, p4, color);

    // 縦の柱
    DrawLine(p0, p4, color); 
    DrawLine(p1, p5, color);
    DrawLine(p2, p6, color);
    DrawLine(p3, p7, color);
}

void DebugDraw::DrawOBB(const Vector3& center, const Vector3& size, const Matrix4x4& rotationMat, const Vector4& color)
{
    // 原点中心AABBを回転・平行移動
    Vector3 half = size * 0.5f;
    Vector3 v[8] = {
        {-half.x,-half.y,-half.z},{ half.x,-half.y,-half.z},
        { half.x,-half.y, half.z},{-half.x,-half.y, half.z},
        {-half.x, half.y,-half.z},{ half.x, half.y,-half.z},
        { half.x, half.y, half.z},{-half.x, half.y, half.z}
    };
    for (int i = 0; i < 8; ++i) v[i] = rotationMat.TransformVector(v[i]) + center;

    // 線を描画
    DrawLine(v[0], v[1], color);
    DrawLine(v[1], v[2], color);
    DrawLine(v[2], v[3], color);
    DrawLine(v[3], v[0], color);
    DrawLine(v[4], v[5], color);
    DrawLine(v[5], v[6], color);
    DrawLine(v[6], v[7], color);
    DrawLine(v[7], v[4], color);
    DrawLine(v[0], v[4], color); 
    DrawLine(v[1], v[5], color); 
    DrawLine(v[2], v[6], color);
    DrawLine(v[3], v[7], color);
}

void DebugDraw::DrawSphere(const Vector3& center, float radius, const Vector4& color)
{
    // XY, YZ, ZX平面の円で球
    const int seg = 16;
    const float step = 3.14159265f * 2.0f / seg;
    for (int i = 0; i < seg; ++i)
    {
        float a1 = i * step, a2 = (i + 1) * step;
        DrawLine({ center.x + cosf(a1) * radius, center.y + sinf(a1) * radius, center.z },
            { center.x + cosf(a2) * radius, center.y + sinf(a2) * radius, center.z }, color);
        DrawLine({ center.x, center.y + cosf(a1) * radius, center.z + sinf(a1) * radius },
            { center.x, center.y + cosf(a2) * radius, center.z + sinf(a2) * radius }, color);
        DrawLine({ center.x + sinf(a1) * radius, center.y, center.z + cosf(a1) * radius },
            { center.x + sinf(a2) * radius, center.y, center.z + cosf(a2) * radius }, color);
    }
}

void DebugDraw::DrawFrustum(const Matrix4x4& viewProj, const Vector4& color)
{
    // NDCの8頂点をワールド変換
    std::vector<Vector3> ndc = 
    {
        {-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0},
        {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
    };
    Matrix4x4 invVP = Matrix4x4::Inverse(viewProj);
    std::vector<Vector3> wpts;
    for (auto& p : ndc)
    {
        Vector4 t = invVP.Transform({ p.x,p.y,p.z,1 });
        if (t.w != 0) t /= t.w;
        wpts.push_back({ t.x,t.y,t.z });
    }

    // 線を結ぶ
    DrawLine(wpts[0], wpts[1], color);
    DrawLine(wpts[1], wpts[2], color);
    DrawLine(wpts[2], wpts[3], color);
    DrawLine(wpts[3], wpts[0], color);
    DrawLine(wpts[4], wpts[5], color);
    DrawLine(wpts[5], wpts[6], color);
    DrawLine(wpts[6], wpts[7], color); 
    DrawLine(wpts[7], wpts[4], color);
    DrawLine(wpts[0], wpts[4], color);
    DrawLine(wpts[1], wpts[5], color);
    DrawLine(wpts[2], wpts[6], color); 
    DrawLine(wpts[3], wpts[7], color);
}

}

#endif