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

void DebugDraw::DrawFrustum(const Matrix4x4& viewProjectionMatrix, const Vector4& color)
{
    // 8つの頂点（NDC座標: -1.0 ~ 1.0）
    // Direct3Dの場合、Zは 0.0(Near) ～ 1.0(Far)
    // OpenGLの場合は -1.0(Near) ～ 1.0(Far) ですが、今回は一般的なD3D系と仮定します
    std::vector<Vector3> ndcPoints = {
        // Near Plane (z = 0)
        {-1.0f, -1.0f, 0.0f}, { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f}, {-1.0f,  1.0f, 0.0f},
        // Far Plane (z = 1)
        {-1.0f, -1.0f, 1.0f}, { 1.0f, -1.0f, 1.0f},
        { 1.0f,  1.0f, 1.0f}, {-1.0f,  1.0f, 1.0f}
    };

    // 逆行列を計算
    Matrix4x4 inverseVP = Matrix4x4::Inverse(viewProjectionMatrix);

    std::vector<Vector3> worldPoints;
    for (const auto& p : ndcPoints)
    {
        // 座標変換 (Transform)
        // ここでは同次座標系(w)の計算を行い、w除算(透視投影変換の逆)をする必要があります
        Vector4 pos4 = { p.x, p.y, p.z, 1.0f };

        // 行列との掛け算 (実装に合わせて Matrix * Vec か Vec * Matrix か確認してください)
        // ここでは Vector4 * Matrix4x4 と仮定
        Vector4 transformed = inverseVP.Transform(pos4);

        // w除算してワールド座標へ
        if (transformed.w != 0.0f) {
            transformed.x /= transformed.w;
            transformed.y /= transformed.w;
            transformed.z /= transformed.w;
        }
        worldPoints.push_back({ transformed.x, transformed.y, transformed.z });
    }

    // 線を結ぶ
    // Near面
    DrawLine(worldPoints[0], worldPoints[1], color);
    DrawLine(worldPoints[1], worldPoints[2], color);
    DrawLine(worldPoints[2], worldPoints[3], color);
    DrawLine(worldPoints[3], worldPoints[0], color);

    // Far面
    DrawLine(worldPoints[4], worldPoints[5], color);
    DrawLine(worldPoints[5], worldPoints[6], color);
    DrawLine(worldPoints[6], worldPoints[7], color);
    DrawLine(worldPoints[7], worldPoints[4], color);

    // NearとFarを結ぶ柱
    DrawLine(worldPoints[0], worldPoints[4], color);
    DrawLine(worldPoints[1], worldPoints[5], color);
    DrawLine(worldPoints[2], worldPoints[6], color);
    DrawLine(worldPoints[3], worldPoints[7], color);
}

#endif