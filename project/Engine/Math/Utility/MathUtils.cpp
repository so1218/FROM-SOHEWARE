#define _USE_MATH_DEFINES
#include <cmath> 
#include <cfloat>

#include "MathUtils.h"
#include "WorldTransform.h"

namespace Math
{
    Vector3 Project(
        const Vector3 worldPosition,
        float viewportX, float viewportY, float viewportWidth, float viewportHeight,
        const Matrix4x4 viewProjection)
    {
        // ワールド座標をクリップ座標に変換
        Vector4 clipPos = viewProjection * Vector4(worldPosition.x, worldPosition.y, worldPosition.z, 1.0f);

        // クリップ座標を正規化デバイス座標(NDC)に変換(w除算)
        Vector3 ndcPos;
        if (clipPos.w != 0.0f)
        {
            ndcPos.x = clipPos.x / clipPos.w;
            ndcPos.y = clipPos.y / clipPos.w;
            ndcPos.z = clipPos.z / clipPos.w;
        }
        else 
        {
            return Vector3(-FLT_MAX, -FLT_MAX, 0.0f);
        }

        // 正規化デバイス座標(NDC)をビューポート座標(スクリーン座標)に変換
        Vector3 screenPos;
        screenPos.x = (ndcPos.x + 1.0f) * 0.5f * viewportWidth + viewportX;
        screenPos.y = (1.0f - ndcPos.y) * 0.5f * viewportHeight + viewportY; // Y軸反転
        screenPos.z = ndcPos.z;

        return screenPos;
    }

    Vector4 Uint32ToColorVector(uint32_t color)
    {
        float r = ((color >> 24) & 0xFF) / 255.0f;
        float g = ((color >> 16) & 0xFF) / 255.0f;
        float b = ((color >> 8) & 0xFF) / 255.0f;
        float a = (color & 0xFF) / 255.0f;

        return { r, g, b, a };
    }

    uint32_t ColorVectorToUint32(const Vector4& color)
    {
        uint32_t r = static_cast<uint32_t>(color.x * 255.0f) & 0xFF;
        uint32_t g = static_cast<uint32_t>(color.y * 255.0f) & 0xFF;
        uint32_t b = static_cast<uint32_t>(color.z * 255.0f) & 0xFF;
        uint32_t a = static_cast<uint32_t>(color.w * 255.0f) & 0xFF;

        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    float RandomFloat(float min, float max)
    {
        if (min >= max)
        {
            return min;
        }

        return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
    }

    int RandomInt(int min, int max)
    {
        if (min >= max)
        {
            return min;
        }

        return min + (rand() % (max - min + 1));
    }

    float ToRadians(float degrees)
    {
        return degrees * (float(Math::PI) / 180.0f);
    }

    Vector3 CrossProduct(const Vector3& v1, const Vector3& v2)
    {
        return
        {
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        };
    }

    Vector2 WorldToScreen(const Vector3& worldPos, const Matrix4x4& viewProjection, float screenWidth, float screenHeight)
    {
        // クリップ座標へ変換
        Vector4 clipPos = viewProjection * Vector4(worldPos.x, worldPos.y, worldPos.z, 1.0f);

        // カメラ背面にある場合は無効値を返す
        if (clipPos.w <= 0.0f)
        {
            return { -10000.0f, -10000.0f };
        }

        // NDC (正規化デバイス座標) へ変換
        Vector3 ndcPos = 
        {
            clipPos.x / clipPos.w,
            clipPos.y / clipPos.w,
            clipPos.z / clipPos.w
        };

        // スクリーン座標へ変換
        float screenX = (ndcPos.x + 1.0f) * 0.5f * screenWidth;
        float screenY = (1.0f - ndcPos.y) * 0.5f * screenHeight;

        return { screenX, screenY };
    }
}