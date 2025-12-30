#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include <vector>
#include <cstdint>

class Renderer;
class Camera;

/// デバッグ描画
class DebugDraw
{
public:
    // フレーム開始時にカメラをセット (描画で使うため)
    static void SetCamera(Camera* camera);

#ifdef _DEBUG
    static void Initialize(Renderer* renderer);
    static void DrawLine(const Vector3& start, const Vector3& end, const Vector4& color);
    static void DrawAABB(const Vector3& min, const Vector3& max, const Vector4& color);
    static void DrawOBB(const Vector3& center, const Vector3& size, const Matrix4x4& rot, const Vector4& color);
    static void DrawSphere(const Vector3& center, float radius, const Vector4& color);
   
#else
    // リリース時は中身のないインライン関数に置換される
    // コンパイラの最適化で呼び出し自体が消滅する（コストゼロ）
    static inline void Initialize(Renderer*) {}
    static inline void DrawLine(const Vector3&, const Vector3&, const Vector4&) {}
    static inline void DrawAABB(const Vector3&, const Vector3&, const Vector4&) {}
    static inline void DrawOBB(const Vector3&, const Vector3&, const Matrix4x4&, const Vector4&) {}
    static inline void DrawSphere(const Vector3&, float, const Vector4&) {}
#endif

private:
    // 内部で保持するポインタ
    static Renderer* renderer_;
    static Camera* camera_;
};
