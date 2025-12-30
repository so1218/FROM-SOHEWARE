#include "LightManager.h"
#include "Structures.h"
#include "DebugDraw.h"

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
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
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
    if (availablePointLightIndices_.empty()) {
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
    if (availableAreaLightIndices_.empty()) {
        return -1; // 利用可能なスロットがない
    }
    int index = availableAreaLightIndices_.front();
    availableAreaLightIndices_.pop();

    areaLightData_[index].enable = true; // ライトを有効化
    return index;
}

void LightManager::ReturnPointLight(int index)
{
    if (index < 0 || index >= pointLightCount_) {
        return; // 無効なインデックス
    }

    pointLightData_[index].enable = false; // ライトを無効化
    // 念のためデータをリセット
    pointLightData_[index].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    pointLightData_[index].intensity = 0.0f;

    availablePointLightIndices_.push(index); // キューに戻す
}

void LightManager::ReturnAreaLight(int index)
{
    if (index < 0 || index >= areaLightCount_) {
        return; // 無効なインデックス
    }
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

void LightManager::DrawDebugLights()
{
#ifdef _DEBUG
    // --- 1. Point Light の描画 ---
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        if (!pointLightData_[i].enable) continue;

        // ライトの色をデバッグ線の色にも反映させると分かりやすい
        Vector4 color = pointLightData_[i].color;
        color.w = 1.0f; // アルファは不透明に

        // 半径を表す球を描画
        DebugDraw::DrawSphere(pointLightData_[i].position, pointLightData_[i].radius, color);
    }

    // --- 2. Spot Light の描画 (円錐を描く) ---
    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
    {
        if (!spotLightData_[i].enable) continue;

        Vector4 color = spotLightData_[i].color;
        color.w = 1.0f;

        Vector3 startPos = spotLightData_[i].position;
        Vector3 dir = -spotLightData_[i].direction;
        float dist = spotLightData_[i].distance;
        float angleCos = spotLightData_[i].cosAngle;

        // 円錐の底面の中心
        Vector3 baseCenter = startPos + (dir * dist);

        // 円錐の底面の半径 (三角関数: 半径 = 距離 * tan(acos(cosAngle)))
        // acosとか重いので簡易的に計算しても良いが、正確にはこう
        float angle = acosf(angleCos);
        float radius = dist * tanf(angle);

        // 中心線を引く
        DebugDraw::DrawLine(startPos, baseCenter, color);

        // 底面の円を描くための軸作成 (dirに垂直なベクトルを見つける)
        Vector3 up = { 0, 1, 0 };
        if (fabsf(dir.y) > 0.99f) up = { 1, 0, 0 }; // dirが真上ならX軸を仮の右とする
        Vector3 right = Math::CrossProduct(up, dir); // 修正: Math::CrossProductが必要
        up = Math::CrossProduct(dir, right); // 正確な上方向

        // 円を簡易的に描画 (16分割)
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
            // 始点から円周への線（4本に1本くらい引くと円錐っぽくなる）
            if (j % 4 == 0) {
                DebugDraw::DrawLine(startPos, p1, color);
            }
        }
    }

    // --- 3. Area Light の描画 (長方形を描く) ---
    for (int i = 0; i < MAX_AREA_LIGHTS; ++i)
    {
        if (!areaLightData_[i].enable) continue;

        Vector4 color = areaLightData_[i].color;
        color.w = 1.0f;

        Vector3 pos = areaLightData_[i].position;
        Vector3 right = areaLightData_[i].right; // 中心から端までのベクトルと仮定
        Vector3 up = areaLightData_[i].up;       // 中心から上までのベクトルと仮定

        // 4つの頂点を計算 (Center +/- Right +/- Up)
        Vector3 p0 = pos - right - up; // 左下
        Vector3 p1 = pos + right - up; // 右下
        Vector3 p2 = pos + right + up; // 右上
        Vector3 p3 = pos - right + up; // 左上

        // 枠線を描画
        DebugDraw::DrawLine(p0, p1, color);
        DebugDraw::DrawLine(p1, p2, color);
        DebugDraw::DrawLine(p2, p3, color);
        DebugDraw::DrawLine(p3, p0, color);

        // どっちが「表」か分かるように法線も引くと親切
        Vector3 normal = Math::CrossProduct(right, up); 
    }

#endif
}