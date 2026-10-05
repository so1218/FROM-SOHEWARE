#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"

namespace FE
{

class RendererManager;
class Camera;

// デバッグ描画
class DebugDraw
{
public:
#ifdef ENABLE_DEBUG_DRAW
    static void Initialize(RendererManager* rendererManager);

    // デバッグ描画全体のオン/オフ制御
    static void SetEnabled(bool enabled) { enabled_ = enabled; }
    static bool IsEnabled() { return enabled_; }

    // デバッグ用描画
    static void DrawLine(const Vector3& start, const Vector3& end, const Vector4& color);       // 線分
    static void DrawAABB(const Vector3& min, const Vector3& max, const Vector4& color);         // 軸平行ボックス
    static void DrawOBB(const Vector3& center, const Vector3& size, const Matrix4x4& rot, const Vector4& color); // 任意回転ボックス
    static void DrawSphere(const Vector3& center, float radius, const Vector4& color);          // 球
    static void DrawFrustum(const Matrix4x4& viewProjectionMatrix, const Vector4& color);       // カメラ視錐台
#else
    // リリース時は無効化
    static inline void Initialize(RendererManager*) {}
    static inline void DrawLine(const Vector3&, const Vector3&, const Vector4&) {}
    static inline void DrawAABB(const Vector3&, const Vector3&, const Vector4&) {}
    static inline void DrawOBB(const Vector3&, const Vector3&, const Matrix4x4&, const Vector4&) {}
    static inline void DrawSphere(const Vector3&, float, const Vector4&) {}
    static inline void DrawFrustum(const Matrix4x4&, const Vector4&) {}
#endif

private:
#ifdef ENABLE_DEBUG_DRAW
    static RendererManager* rendererManager_;
    static bool enabled_; 
#endif
};

}