#include "pch.h"
#include "LightManager.h"
#include "Structures.h"
#include "DebugDraw.h"
#include "ImGuiManager.h"

namespace FE
{

void LightManager::Initialize(ID3D12Device* device)
{
    // Directional Light Buffer
    directionalLightResource_ = BufferManager::CreateBufferResource(
        device, sizeof(DirectionalLight) * MAX_DIRECTIONAL_LIGHTS);
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));

    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i)
    {
        directionalLightData_[i].enable = false;
        directionalLightData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
        directionalLightData_[i].direction = { 0.0f, -1.0f, 1.25f };
        directionalLightData_[i].intensity = 1.0f;
        directionalLightData_[i].volumetricScatteringIntensity = 5.0f;
        directionalLightPositions_[i] = { 0.0f, 10.0f, 0.0f };
    }

    // Point Light Buffer
    pointLightResource_ = BufferManager::CreateBufferResource(
        device, sizeof(PointLight) * MAX_POINT_LIGHTS);
    pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

    // Spot Light Buffer
    spotLightResource_ = BufferManager::CreateBufferResource(
        device, sizeof(SpotLight) * MAX_SPOT_LIGHTS);
    spotLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData_));

    // Shadow Buffer
    shadowDataResource_ = BufferManager::CreateMappedConstantBuffer(device, &shadowData_);

    for (int i = 0; i < MAX_CASCADE_COUNT; ++i)
    {
        shadowData_->cascadeLightViewProj[i] = Matrix4x4::MakeIdentity();
    }
    shadowData_->cascadeSplits = { 0.0f, 0.0f, 0.0f, 0.0f };

    BeginFrame();
}

void LightManager::BeginFrame()
{
    // カウントを 0 にリセット
    activePointLightCount_ = 0;
    activeSpotLightCount_ = 0;

    // 全ライトの有効化フラグをリセット
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        pointLightData_[i].enable = false;
    }
    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
    {
        spotLightData_[i].enable = false;
    }
}

bool LightManager::SubmitPointLight(
    const Vector3& position,
    const Vector4& color,
    float intensity,
    float radius,
    float volumetricScatteringIntensity)
{
    if (activePointLightCount_ >= MAX_POINT_LIGHTS)
    {
        return false;
    }

    int index = activePointLightCount_++;
    auto& light = pointLightData_[index];

    light.enable = true;
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    light.volumetricScatteringIntensity = volumetricScatteringIntensity;

    return true;
}

bool LightManager::SubmitSpotLight(
    const Vector3& position,
    const Vector4& color,
    float intensity,
    float distance,
    const Vector3& direction,
    float cosAngle,
    float volumetricScatteringIntensity)
{
    if (activeSpotLightCount_ >= MAX_SPOT_LIGHTS)
    {
        return false;
    }

    int index = activeSpotLightCount_++;
    auto& light = spotLightData_[index];

    light.enable = true;
    light.position = position;
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    light.distance = distance;
    light.cosAngle = cosAngle;
    light.volumetricScatteringIntensity = volumetricScatteringIntensity;

    return true;
}

void LightManager::SetDirectionalLight(
    int index,
    const Vector3& direction,
    const Vector4& color,
    float intensity,
    float volumetricScatteringIntensity)
{
    if (index < 0 || index >= MAX_DIRECTIONAL_LIGHTS) return;

    directionalLightData_[index].enable = true;
    directionalLightData_[index].direction = direction;
    directionalLightData_[index].color = color;
    directionalLightData_[index].intensity = intensity;
    directionalLightData_[index].volumetricScatteringIntensity = volumetricScatteringIntensity;
}

void LightManager::UpdateDirectionalLightShadowMatrix(int index, const Matrix4x4& viewProjection)
{
    if (index < 0 || index >= directionalLightCount_) return;
    directionalLightData_[index].viewProj = viewProjection;
}

void LightManager::UpdateShadowMatrix(int lightIndex, const Vector3& shadowTarget)
{
    if (lightIndex < 0 || lightIndex >= MAX_DIRECTIONAL_LIGHTS) return;

    auto dirLights = GetDirectionalLightData();
    if (!dirLights || !dirLights[lightIndex].enable) return;

    Vector3 lightDir = dirLights[lightIndex].direction.Normalize();

    float distance = 100.0f;
    Vector3 lightPos = shadowTarget - (lightDir * distance);

    Vector3 up = { 0.0f, 1.0f, 0.0f };
    if (std::abs(lightDir.y) > 0.99f) up = { 1.0f, 0.0f, 0.0f };

    Matrix4x4 lightView = Matrix4x4::MakeLookAt(lightPos, shadowTarget, up);

    float size = 100.0f;
    float nearZ = -100.0f;
    float farZ = 200.0f;
    Matrix4x4 lightProj = Matrix4x4::MakeOrthographic(size, size, nearZ, farZ);

    Matrix4x4 lightViewProj = lightView * lightProj;
    UpdateDirectionalLightShadowMatrix(lightIndex, lightViewProj);
}

void LightManager::UpdateCascadedShadows(
    const Vector3& lightDir,
    const Matrix4x4& cameraView,
    const Matrix4x4& cameraProj,
    float cameraNear,
    float cameraFar)
{
    const float shadowMapResolution = static_cast<float>(SHADOW_MAP_RESOLUTION);
    Vector3 normLightDir = lightDir.Normalize();

    Matrix4x4 invCamViewProj = Matrix4x4::Inverse(cameraView * cameraProj);

    float splits[MAX_CASCADE_COUNT + 1];
    splits[0] = cameraNear;
    splits[MAX_CASCADE_COUNT] = cameraFar;

    const float lambda = 0.5f;

    for (int i = 1; i < MAX_CASCADE_COUNT; ++i)
    {
        float fraction = static_cast<float>(i) / static_cast<float>(MAX_CASCADE_COUNT);

        float logSplit = cameraNear * std::pow(cameraFar / cameraNear, fraction);
        float linSplit = cameraNear + (cameraFar - cameraNear) * fraction;

        splits[i] = lambda * logSplit + (1.0f - lambda) * linSplit;
    }

    shadowData_->cascadeSplits = Vector4{ splits[1], splits[2], splits[3], 0.0f };

    for (int i = 0; i < MAX_CASCADE_COUNT; ++i)
    {
        float nearDist = splits[i];
        float farDist = splits[i + 1];

        float m22 = cameraProj.m[2][2];
        float m32 = cameraProj.m[3][2];
        float minZ = (nearDist * m22 + m32) / nearDist;
        float maxZ = (farDist * m22 + m32) / farDist;

        Vector3 frustumCorners[8] = {
            { -1.0f,  1.0f, minZ }, {  1.0f,  1.0f, minZ }, {  1.0f, -1.0f, minZ }, { -1.0f, -1.0f, minZ },
            { -1.0f,  1.0f, maxZ }, {  1.0f,  1.0f, maxZ }, {  1.0f, -1.0f, maxZ }, { -1.0f, -1.0f, maxZ }
        };

        Vector3 center{ 0.0f, 0.0f, 0.0f };
        for (int j = 0; j < 8; ++j)
        {
            frustumCorners[j] = invCamViewProj.TransformPoint(frustumCorners[j]);
            center = center + frustumCorners[j];
        }
        center = center * (1.0f / 8.0f);

        float radius = 0.0f;
        for (int j = 0; j < 8; ++j)
        {
            float distance = (frustumCorners[j] - center).Length();
            radius = (std::max)(radius, distance);
        }
        radius = std::ceil(radius * 1.1f);

        Vector3 up = { 0.0f, 1.0f, 0.0f };
        if (std::abs(normLightDir.y) > 0.99f) up = { 1.0f, 0.0f, 0.0f };

        Vector3 lightPos = center - (normLightDir * radius);
        Matrix4x4 lightView = Matrix4x4::MakeLookAt(lightPos, center, up);

        float shadowNearZ = -radius * 2.0f;
        float shadowFarZ = radius * 2.0f;
        Matrix4x4 shadowProj = Matrix4x4::MakeOrthographic(radius * 2.0f, radius * 2.0f, shadowNearZ, shadowFarZ);
        Matrix4x4 shadowViewProj = lightView * shadowProj;

        Vector3 shadowOrigin = { 0.0f, 0.0f, 0.0f };
        shadowOrigin = shadowViewProj.TransformPoint(shadowOrigin);
        shadowOrigin = shadowOrigin * (shadowMapResolution / 2.0f);

        Vector3 roundedOrigin{ std::round(shadowOrigin.x), std::round(shadowOrigin.y), std::round(shadowOrigin.z) };
        Vector3 roundOffset = roundedOrigin - shadowOrigin;
        roundOffset = roundOffset * (2.0f / shadowMapResolution);

        shadowProj.m[3][0] += roundOffset.x;
        shadowProj.m[3][1] += roundOffset.y;

        shadowData_->cascadeLightViewProj[i] = lightView * shadowProj;
    }
}

void LightManager::DrawDebugLights()
{
#ifdef ENABLE_DEBUG_DRAW
    // Directional Lightの描画
    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i)
    {
        if (!directionalLightData_[i].enable) continue;

        Vector4 color = directionalLightData_[i].color;
        color.w = 1.0f;

        // 仮想的な位置（太陽の位置）
        Vector3 virtualPos = directionalLightPositions_[i];

        // 方向ベクトルの正規化
        Vector3 dir = directionalLightData_[i].direction.Normalize();

        // 座標軸の作成
        Vector3 up = { 0.0f, 1.0f, 0.0f };
        if (fabsf(dir.y) > 0.99f) up = { 1.0f, 0.0f, 0.0f };
        Vector3 right = Math::CrossProduct(up, dir);
        up = Math::CrossProduct(dir, right).Normalize();

        // 描画パラメータ
        float sunRadius = 0.5f;   // 中心の球の大きさ
        float ringRadius = 1.5f;  // 光の束の太さ
        float rayLength = 5.0f;   // 光の線の長さ
        int segments = 8;         // 円の分割数

        // 中心の太陽
        DebugDraw::DrawSphere(virtualPos, sunRadius, color);

        // 光の束（円筒状の平行線）
        float step = (Math::PI * 2.0f) / segments;
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
        up = Math::CrossProduct(dir, right).Normalize();

        // 円を描画
        const int segments = 16;
        float step = (Math::PI * 2.0f) / segments;
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

#endif
}

void LightManager::DrawSelectedLightGizmo()
{
#ifdef ENABLE_IMGUI
    if (selectedLightType_ == SelectedLightType::None || selectedLightIndex_ < 0) return;

    Matrix4x4 lightMat;
    Vector3 scale = { 1, 1, 1 };
    Vector3 rot = { 0, 0, 0 };
    Vector3 pos = { 0, 0, 0 };

    // 現在のライトの値からダミーの行列を作る
    if (selectedLightType_ == SelectedLightType::Point) {
        PointLight& light = pointLightData_[selectedLightIndex_];
        pos = light.position;
        scale = { light.radius, light.radius, light.radius }; // スケールを半径に
        lightMat = Matrix4x4::MakeAffine(scale, rot, pos);
    }
    else if (selectedLightType_ == SelectedLightType::Spot) {
        SpotLight& light = spotLightData_[selectedLightIndex_];
        pos = light.position;
        lightMat = Matrix4x4::MakeFromDirection(light.direction, pos);
    }
    else if (selectedLightType_ == SelectedLightType::Directional) {
        DirectionalLight& light = directionalLightData_[selectedLightIndex_];
        Vector3 pos = directionalLightPositions_[selectedLightIndex_];
        lightMat = Matrix4x4::MakeFromDirection(light.direction, pos);
    }

    // Gizmoを描画・操作
    if (ImGuiManager::DrawGizmoMatrix(lightMat))
    {
        // 操作された場合、行列を分解してライトに書き戻す
        Vector3 outPos, outRotDeg, outScale;
        ImGuizmo::DecomposeMatrixToComponents(&lightMat.m[0][0], &outPos.x, &outRotDeg.x, &outScale.x);

        if (selectedLightType_ == SelectedLightType::Point) {
            PointLight& light = pointLightData_[selectedLightIndex_];
            light.position = outPos;
            // Scaleの適用（X,Y,Zの平均値を半径にする）
            light.radius = std::max(0.1f, (outScale.x + outScale.y + outScale.z) / 3.0f);
        }
        else if (selectedLightType_ == SelectedLightType::Spot) {
            SpotLight& light = spotLightData_[selectedLightIndex_];
            light.position = outPos;

            // Rotateの適用：行列のZ軸成分が前方ベクトル
            Vector3 newDir = { lightMat.m[2][0], lightMat.m[2][1], lightMat.m[2][2] };
            light.direction = newDir.Normalize();
        }
        else if (selectedLightType_ == SelectedLightType::Directional) {
            DirectionalLight& light = directionalLightData_[selectedLightIndex_];

            // 仮想位置を更新
            directionalLightPositions_[selectedLightIndex_] = outPos;

            // 回転結果の行列のZ軸から新しい向きを計算
            Vector3 newDir = { lightMat.m[2][0], lightMat.m[2][1], lightMat.m[2][2] };
            light.direction = newDir.Normalize();
        }
    }
#endif
}

}