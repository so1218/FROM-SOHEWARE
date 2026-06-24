#include "pch.h"
#include "LightManager.h"
#include "Structures.h"
#include "DebugDraw.h"

namespace FE
{

void LightManager::Initialize(ID3D12Device* device)
{
    // Directional Light
    directionalLightResource_ = BufferManager::CreateBufferResource(
        device, sizeof(DirectionalLight) * MAX_DIRECTIONAL_LIGHTS);
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));

    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i)
    {
        directionalLightData_[i].enable = false;
        directionalLightData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
        directionalLightData_[i].direction = { 0.0f, -1.0f, 1.25f };
        directionalLightData_[i].intensity = 1.0f;
    }

    // Point Light
    pointLightResource_ = BufferManager::CreateBufferResource(
        device, sizeof(PointLight) * MAX_POINT_LIGHTS);
    pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        pointLightData_[i].enable = false;
        pointLightData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
        pointLightData_[i].position = { -4.0f + i * 2.0f, 5.0f, 0.0f };
        pointLightData_[i].intensity = 5.0f;
        pointLightData_[i].radius = 10.0f;
        pointLightData_[i].decay = 3.0f;
        pointLightData_[i].VolumetricScatteringIntensity = 1.0f;
    }

    // Spot Light
    spotLightResource_ = BufferManager::CreateBufferResource(
        device, sizeof(SpotLight) * MAX_SPOT_LIGHTS);
    spotLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData_));

    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
    {
        spotLightData_[i].enable = false;
        spotLightData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
        spotLightData_[i].position = { 0.0f, 5.0f, float(i * 2) };
        spotLightData_[i].intensity = 5.0f;
        spotLightData_[i].direction = { 0.0f, -1.0f, 0.0f };
        spotLightData_[i].distance = 20.0f;
        spotLightData_[i].decay = 3.0f;
        spotLightData_[i].cosAngle = 0.866f;
        spotLightData_[i].VolumetricScatteringIntensity = 8.0f;
    }

    // Area Light
    areaLightResource_ = BufferManager::CreateBufferResource(
        device, sizeof(AreaLight) * MAX_AREA_LIGHTS);
    areaLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&areaLightData_));

    for (int i = 0; i < MAX_AREA_LIGHTS; ++i)
    {
        areaLightData_[i].enable = false;
        areaLightData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
        areaLightData_[i].position = { 0.0f, 2.0f, 0.0f };
        areaLightData_[i].right = { 2.0f, 0.0f, 0.0f }; 
        areaLightData_[i].up = { 0.0f, 0.0f, 1.0f };    
        areaLightData_[i].intensity = 10.0f;
        areaLightData_[i].range = 20.0f;
        areaLightData_[i].decay = 2.0f;
    }

    // 利用可能なインデックスキューを初期化
    availablePointLightIndices_ = {}; // キューをクリア
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        availablePointLightIndices_.push(i);
        pointLightData_[i].enable = false; // 全てのライトを非アクティブで初期化
    }

    // Area Light のキュー初期化
    availableAreaLightIndices_ = {}; // キューをクリア
    for (int i = 0; i < MAX_AREA_LIGHTS; ++i) {
        availableAreaLightIndices_.push(i);
        areaLightData_[i].enable = false; // 全て非アクティブで初期化
    }
}

int LightManager::RequestPointLight()
{
    if (availablePointLightIndices_.empty())
    {
        // 利用可能なライトスロットがない
        return -1;
    }
    int index = availablePointLightIndices_.front();
    availablePointLightIndices_.pop();

    pointLightData_[index].enable = true; // ライトを有効化
    return index;
}

int LightManager::RequestAreaLight()
{
    if (availableAreaLightIndices_.empty()) 
    {
        return -1; // 利用可能なスロットがない
    }
    int index = availableAreaLightIndices_.front();
    availableAreaLightIndices_.pop();

    areaLightData_[index].enable = true; // ライトを有効化
    return index;
}

void LightManager::ReturnPointLight(int index)
{
    if (index < 0 || index >= pointLightCount_)
    {
        return; // 無効なインデックス
    }

    if (!pointLightData_[index].enable) return;

    pointLightData_[index].enable = false; // ライトを無効化
    pointLightData_[index].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    pointLightData_[index].intensity = 0.0f;

    availablePointLightIndices_.push(index); // キューに戻す
}

void LightManager::ReturnAreaLight(int index)
{
    if (index < 0 || index >= areaLightCount_)
    {
        return; // 無効なインデックス
    }

    if (!areaLightData_[index].enable) return;

    areaLightData_[index].enable = false; // ライトを無効化
    availableAreaLightIndices_.push(index); // キューに戻す
}

void LightManager::UpdatePointLightPosition(int index, const Vector3& position)
{
    if (index < 0 || index >= pointLightCount_ || !pointLightData_[index].enable) return;
    pointLightData_[index].position = position;
}

void LightManager::UpdatePointLightProperties(int index, const Vector4& color, float intensity, float radius, float decay)
{
    if (index < 0 || index >= pointLightCount_ || !pointLightData_[index].enable) return;
    pointLightData_[index].color = color;
    pointLightData_[index].intensity = intensity;
    pointLightData_[index].radius = radius;
    pointLightData_[index].decay = decay;
}

void LightManager::UpdateAreaLightProperties(int index, const Vector4& color, float intensity,
    const Vector3& position, const Vector3& right, const Vector3& up,
    float range, float decay)
{
    if (index < 0 || index >= areaLightCount_ || !areaLightData_[index].enable) return;
    areaLightData_[index].color = color;
    areaLightData_[index].intensity = intensity;
    areaLightData_[index].position = position;
    areaLightData_[index].right = right;
    areaLightData_[index].up = up;
    areaLightData_[index].range = range;
    areaLightData_[index].decay = decay;
}

void LightManager::UpdateDirectionalLightShadowMatrix(int index, const Matrix4x4& viewProjection)
{
    // 範囲チェック
    if (index < 0 || index >= directionalLightCount_) return;

    // マップ済みのメモリに直接書き込む
    directionalLightData_[index].viewProj = viewProjection;
}

void LightManager::UpdateShadowMatrix(int lightIndex, const Vector3& shadowTarget)
{
    auto* dirLights = GetDirectionalLightData();
    if (!dirLights[lightIndex].enable) return;

    // ライト方向を正規化
    Vector3 lightDir = dirLights[lightIndex].direction;
    lightDir = lightDir.Normalize();

    // ライト位置を決定
    float distance = 100.0f;
    Vector3 lightPos = shadowTarget - (lightDir * distance);

    // 上方向ベクトル
    Vector3 up = { 0.0f, 1.0f, 0.0f };
    if (fabs(lightDir.y) > 0.99f) up = { 1.0f, 0.0f, 0.0f };

    // ライトのビュー行列を作成
    Matrix4x4 lightView = Matrix4x4::MakeLookAt(lightPos, shadowTarget, up);

    // 平行光源用の正射影行列を作成
    float size = 100.0f;
    float nearZ = -100.0f;
    float farZ = 200.0f;
    Matrix4x4 lightProj = Matrix4x4::MakeOrthographic(size, size, nearZ, farZ);

    // ビュー行列と射影行列を合成
    Matrix4x4 lightViewProj = lightView * lightProj;

    // シャドウ行列を更新
    UpdateDirectionalLightShadowMatrix(lightIndex, lightViewProj);
}

void LightManager::DrawDebugLights()
{
#ifdef IS_DEVELOPMENT
    // Directional Lightの描画
    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i)
    {
        if (!directionalLightData_[i].enable) continue;

        Vector4 color = directionalLightData_[i].color;
        color.w = 1.0f;

        // 仮想的な位置（太陽の位置）
        Vector3 virtualPos = { float(i) * 3.0f, 10.0f, 0.0f };

        // 方向ベクトルの正規化
        Vector3 dir = directionalLightData_[i].direction;
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len != 0.0f) {
            dir = { dir.x / len, dir.y / len, dir.z / len };
        }

        // 座標軸の作成
        Vector3 up = { 0.0f, 1.0f, 0.0f };
        if (fabsf(dir.y) > 0.99f) up = { 1.0f, 0.0f, 0.0f };
        Vector3 right = Math::CrossProduct(up, dir);
        up = Math::CrossProduct(dir, right);

        // 描画パラメータ
        float sunRadius = 0.5f;   // 中心の球の大きさ
        float ringRadius = 1.5f;  // 光の束の太さ
        float rayLength = 5.0f;   // 光の線の長さ
        int segments = 8;         // 円の分割数

        // 中心の太陽
        DebugDraw::DrawSphere(virtualPos, sunRadius, color);

        // 光の束（円筒状の平行線）
        float step = (3.141592f * 2.0f) / segments;
        for (int j = 0; j < segments; ++j)
        {
            float theta = j * step;
            float nextTheta = (j + 1) * step;

            // 円周上の点
            Vector3 offset1 = (right * cosf(theta) * ringRadius) + (up * sinf(theta) * ringRadius);
            Vector3 offset2 = (right * cosf(nextTheta) * ringRadius) + (up * sinf(nextTheta) * ringRadius);

            // ワールド座標に変換
            Vector3 p1 = virtualPos + offset1;
            Vector3 p2 = virtualPos + offset2;

            // リングを描く
            DebugDraw::DrawLine(p1, p2, color);

            // リングから伸びる平行線を描く
            Vector3 p1End = p1 + (dir * rayLength);
            DebugDraw::DrawLine(p1, p1End, color);
        }

        // 中心線（太陽からまっすぐ伸びる線）
        DebugDraw::DrawLine(virtualPos, virtualPos + (dir * rayLength), color);
    }

    // Point Lightの描画
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        if (!pointLightData_[i].enable) continue;

        // ライトの色をデバッグ線の色にも反映
        Vector4 color = pointLightData_[i].color;
        color.w = 1.0f;

        // 半径を表す球を描画
        DebugDraw::DrawSphere(pointLightData_[i].position, pointLightData_[i].radius, color);
    }

    // Spot Lightの描画
    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
    {
        if (!spotLightData_[i].enable) continue;

        Vector4 color = spotLightData_[i].color;
        color.w = 1.0f;

        Vector3 startPos = spotLightData_[i].position;
        Vector3 dir = spotLightData_[i].direction;
        float dist = spotLightData_[i].distance;
        float angleCos = spotLightData_[i].cosAngle;

        // 円錐の底面の中心
        Vector3 baseCenter = startPos + (dir * dist);

        // 円錐の底面の半径
        float angle = acosf(angleCos);
        float radius = dist * tanf(angle);

        // 中心線を引く
        DebugDraw::DrawLine(startPos, baseCenter, color);

        // 底面の円を描くための軸作成
        Vector3 up = { 0, 1, 0 };
        if (fabsf(dir.y) > 0.99f) up = { 1, 0, 0 }; 
        Vector3 right = Math::CrossProduct(up, dir); 
        up = Math::CrossProduct(dir, right); 

        // 円を描画
        const int segments = 16;
        float step = (3.141592f * 2.0f) / segments;
        for (int j = 0; j < segments; ++j)
        {
            float theta = j * step;
            float nextTheta = (j + 1) * step;

            // 円周上の点
            Vector3 p1 = baseCenter + (right * (cosf(theta) * radius)) + (up * (sinf(theta) * radius));
            Vector3 p2 = baseCenter + (right * (cosf(nextTheta) * radius)) + (up * (sinf(nextTheta) * radius));

            // 円周を描く
            DebugDraw::DrawLine(p1, p2, color);
            // 始点から円周への線
            if (j % 4 == 0) {
                DebugDraw::DrawLine(startPos, p1, color);
            }
        }
    }

    // Area Lightの描画
    for (int i = 0; i < MAX_AREA_LIGHTS; ++i)
    {
        if (!areaLightData_[i].enable) continue;

        Vector4 color = areaLightData_[i].color;
        color.w = 1.0f;

        Vector3 pos = areaLightData_[i].position;
        Vector3 right = areaLightData_[i].right; 
        Vector3 up = areaLightData_[i].up;       

        // 4つの頂点を計算 
        Vector3 p0 = pos - right - up; // 左下
        Vector3 p1 = pos + right - up; // 右下
        Vector3 p2 = pos + right + up; // 右上
        Vector3 p3 = pos - right + up; // 左上

        // 四角形の外枠
        DebugDraw::DrawLine(p0, p1, color);
        DebugDraw::DrawLine(p1, p2, color);
        DebugDraw::DrawLine(p2, p3, color);
        DebugDraw::DrawLine(p3, p0, color);

        // 照射方向を示す法線
        Vector3 normal = Math::CrossProduct(right, up); 
    }

#endif
}

}