#include "DebugDraw.h"

#ifdef _DEBUG

#include "Renderer.h"
#include "Camera.h"
#include "MathUtils.h" 

// 静的メンバの実体
Renderer* DebugDraw::renderer_ = nullptr;
Camera* DebugDraw::camera_ = nullptr;

void DebugDraw::Initialize(Renderer* renderer)
{
    renderer_ = renderer;
}

void DebugDraw::SetCamera(Camera* camera)
{
    camera_ = camera;
}

void DebugDraw::DrawLine(const Vector3& start, const Vector3& end, const Vector4& color)
{
    if (!renderer_ || !camera_) return;

    // RendererのSubmitLineを呼び出す (色は Vector4 -> uint32_t に変換が必要ならここで行う)
    // ※Renderer::SubmitLineがuint32_tを受け取る仕様なら変換する
    uint32_t colorU = Math::ColorVectorToUint32(color);
    renderer_->SubmitLine(start, end, *camera_, colorU);
}

void DebugDraw::DrawAABB(const Vector3& min, const Vector3& max, const Vector4& color)
{
    // AABBは8つの頂点を持つ直方体。12本の線で構成される。

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
    DrawLine(p0, p1, color); DrawLine(p1, p2, color);
    DrawLine(p2, p3, color); DrawLine(p3, p0, color);

    // 上面の枠
    DrawLine(p4, p5, color); DrawLine(p5, p6, color);
    DrawLine(p6, p7, color); DrawLine(p7, p4, color);

    // 縦の柱
    DrawLine(p0, p4, color); DrawLine(p1, p5, color);
    DrawLine(p2, p6, color); DrawLine(p3, p7, color);
}

void DebugDraw::DrawOBB(const Vector3& center, const Vector3& size, const Matrix4x4& rotationMat, const Vector4& color)
{
    // OBBは「原点中心のAABB」を「回転・移動」させたものと考える
    Vector3 halfSize = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };

    // ローカル空間での8頂点
    Vector3 vertices[8] = {
        { -halfSize.x, -halfSize.y, -halfSize.z }, // 0
        {  halfSize.x, -halfSize.y, -halfSize.z }, // 1
        {  halfSize.x, -halfSize.y,  halfSize.z }, // 2
        { -halfSize.x, -halfSize.y,  halfSize.z }, // 3
        { -halfSize.x,  halfSize.y, -halfSize.z }, // 4
        {  halfSize.x,  halfSize.y, -halfSize.z }, // 5
        {  halfSize.x,  halfSize.y,  halfSize.z }, // 6
        { -halfSize.x,  halfSize.y,  halfSize.z }  // 7
    };

    // すべての頂点を「回転・移動」させる
    for (int i = 0; i < 8; ++i)
    {
        // 回転行列を適用
        vertices[i] = rotationMat.TransformNormal(vertices[i]); // TransformNormalは平行移動を含まない回転のみ
        // 中心座標へ移動
        vertices[i] = vertices[i] + center;
    }

    // 線を結ぶ（インデックスで指定すると楽）
    // 下面
    DrawLine(vertices[0], vertices[1], color); DrawLine(vertices[1], vertices[2], color);
    DrawLine(vertices[2], vertices[3], color); DrawLine(vertices[3], vertices[0], color);
    // 上面
    DrawLine(vertices[4], vertices[5], color); DrawLine(vertices[5], vertices[6], color);
    DrawLine(vertices[6], vertices[7], color); DrawLine(vertices[7], vertices[4], color);
    // 柱
    DrawLine(vertices[0], vertices[4], color); DrawLine(vertices[1], vertices[5], color);
    DrawLine(vertices[2], vertices[6], color); DrawLine(vertices[3], vertices[7], color);
}

void DebugDraw::DrawSphere(const Vector3& center, float radius, const Vector4& color)
{
    // 完全な球は線で描けないので、XY, YZ, ZX 平面の3つの円で表現することが多い
    const int segments = 16;
    const float angleStep = (3.14159265f * 2.0f) / segments;

    // XY平面の円
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        Vector3 p1_xy = { center.x + cosf(angle1) * radius, center.y + sinf(angle1) * radius, center.z };
        Vector3 p2_xy = { center.x + cosf(angle2) * radius, center.y + sinf(angle2) * radius, center.z };
        DrawLine(p1_xy, p2_xy, color);

        Vector3 p1_yz = { center.x, center.y + cosf(angle1) * radius, center.z + sinf(angle1) * radius };
        Vector3 p2_yz = { center.x, center.y + cosf(angle2) * radius, center.z + sinf(angle2) * radius };
        DrawLine(p1_yz, p2_yz, color);

        Vector3 p1_zx = { center.x + sinf(angle1) * radius, center.y, center.z + cosf(angle1) * radius };
        Vector3 p2_zx = { center.x + sinf(angle2) * radius, center.y, center.z + cosf(angle2) * radius };
        DrawLine(p1_zx, p2_zx, color);
    }
}

#endif