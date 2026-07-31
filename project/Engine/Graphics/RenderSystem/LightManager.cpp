#include "pch.h"
#include "LightManager.h"
#include "Structures.h"
#include "DebugDraw.h"
#include "ImGuiManager.h"

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
        directionalLightData_[i].volumetricScatteringIntensity = 5.0f;
        directionalLightPositions_[i] = { 0.0f, 10.0f, 0.0f };
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
        pointLightData_[i].volumetricScatteringIntensity = 1.0f;
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
        spotLightData_[i].cosAngle = 0.866f;
        spotLightData_[i].volumetricScatteringIntensity = 8.0f;
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

    availableSpotLightIndices_ = {};
    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
    {
        availableSpotLightIndices_.push(i);
        spotLightData_[i].enable = false; // 初期状態はすべてオフ
    }

    // Area Light のキュー初期化
    availableAreaLightIndices_ = {}; // キューをクリア
    for (int i = 0; i < MAX_AREA_LIGHTS; ++i) 
    {
        availableAreaLightIndices_.push(i);
        areaLightData_[i].enable = false; // 全て非アクティブで初期化
    }

    // ShadowData
    shadowDataResource_ = BufferManager::CreateMappedConstantBuffer(
        device, &shadowData_);

    // 初期化
    for (int i = 0; i < 4; ++i)
    {
        shadowData_->cascadeLightViewProj[i] = Matrix4x4::MakeIdentity();
    }
    shadowData_->cascadeSplits = { 0.0f, 0.0f, 0.0f, 0.0f };
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

int LightManager::RequestSpotLight()
{
    if (availableSpotLightIndices_.empty()) return -1;

    int index = availableSpotLightIndices_.front();
    availableSpotLightIndices_.pop();

    spotLightData_[index].enable = true;
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

void LightManager::ReturnSpotLight(int index)
{
    if (index < 0 || index >= spotLightCount_) return;
    if (!spotLightData_[index].enable) return;

    spotLightData_[index].enable = false;
    spotLightData_[index].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    spotLightData_[index].intensity = 0.0f;

    availableSpotLightIndices_.push(index);
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

void LightManager::UpdateSpotLightTransform(int index, const Vector3& position, const Vector3& direction)
{
    if (index < 0 || index >= spotLightCount_ || !spotLightData_[index].enable) return;

    spotLightData_[index].position = position;
    // 方向ベクトルは必ず正規化（長さを1に）して代入
    spotLightData_[index].direction = direction;
}

void LightManager::UpdatePointLightProperties(int index, const Vector4& color, float intensity, float radius, float volumetricScatteringIntensity)
{
    if (index < 0 || index >= pointLightCount_ || !pointLightData_[index].enable) return;
    pointLightData_[index].color = color;
    pointLightData_[index].intensity = intensity;
    pointLightData_[index].radius = radius;
    pointLightData_[index].volumetricScatteringIntensity = volumetricScatteringIntensity;
}

void LightManager::UpdateSpotLightProperties(int index, const Vector4& color, float intensity, float distance, float cosAngle, float volumetricScatteringIntensity)
{
    if (index < 0 || index >= spotLightCount_ || !spotLightData_[index].enable) return;

    spotLightData_[index].color = color;
    spotLightData_[index].intensity = intensity;
    spotLightData_[index].distance = distance;
    spotLightData_[index].cosAngle = cosAngle;
    spotLightData_[index].volumetricScatteringIntensity = volumetricScatteringIntensity;
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

void LightManager::UpdateCascadedShadows(
    const Vector3& lightDir,
    const Matrix4x4& cameraView,
    const Matrix4x4& cameraProj,
    float cameraNear,
    float cameraFar)
{
    // シャドウマップの解像度（テクセルスナップ用。ShadowMapの解像度に合わせる）
    const float shadowMapResolution = 2048.0f;

    // 正規化したライトの方向
    Vector3 normLightDir = lightDir.Normalize();

    // カメラの逆ViewProjection行列を計算（NDC空間からワールド空間へ戻すため）
    Matrix4x4 invCamViewProj = Matrix4x4::Inverse(cameraView * cameraProj);

    // カスケードの分割距離の計算 
    float splits[5];
    splits[0] = cameraNear;
    splits[4] = cameraFar;

    // lambda: 0.0で完全線形、1.0で完全対数分割。UEのデフォルトに近い 0.5〜0.7 
    const float lambda = 0.5f;

    for (int i = 1; i < 4; ++i)
    {
        float fraction = static_cast<float>(i) / 4.0f;
        // 対数分割（手前に多く解像度を割く）
        float logSplit = cameraNear * std::pow(cameraFar / cameraNear, fraction);
        // 線形分割（均等に割く）
        float linSplit = cameraNear + (cameraFar - cameraNear) * fraction;

        // ブレンド
        splits[i] = lambda * logSplit + (1.0f - lambda) * linSplit;
    }

    // ピクセルシェーダーでの境界判定にビュー空間のZ距離を送る
    shadowData_->cascadeSplits = Vector4{ splits[1], splits[2], splits[3], splits[4] };

    // 各カスケードの行列を計算
    for (int i = 0; i < 4; ++i)
    {
        float nearDist = splits[i];
        float farDist = splits[i + 1];

        // 各カスケードのプロジェクション空間でのNear/FarのZ値を求める
        // 深度 [0, 1] へのマッピング
        float m22 = cameraProj.m[2][2];
        float m32 = cameraProj.m[3][2];
        float minZ = (nearDist * m22 + m32) / nearDist;
        float maxZ = (farDist * m22 + m32) / farDist;

        // NDC（正規化デバイス座標）での視錐台の8頂点を定義
        Vector3 frustumCorners[8] = {
            { -1.0f,  1.0f, minZ }, {  1.0f,  1.0f, minZ }, {  1.0f, -1.0f, minZ }, { -1.0f, -1.0f, minZ },
            { -1.0f,  1.0f, maxZ }, {  1.0f,  1.0f, maxZ }, {  1.0f, -1.0f, maxZ }, { -1.0f, -1.0f, maxZ }
        };

        // 8頂点をワールド空間に変換し、その中心を求める
        Vector3 center{ 0.0f, 0.0f, 0.0f };
        for (int j = 0; j < 8; ++j)
        {
            frustumCorners[j] = invCamViewProj.TransformPoint(frustumCorners[j]);
            center = center + frustumCorners[j];
        }
        center = center * (1.0f / 8.0f);

        // 外接球の半径を計算
        // カメラが回転してもライトの投影エリアのサイズが変化しなくなり、影のチラツキが消える
        float radius = 0.0f;
        for (int j = 0; j < 8; ++j)
        {
            float distance = (frustumCorners[j] - center).Length();
            radius = (std::max)(radius, distance);
        }
        // わずかにバッファを持たせる
        radius = std::ceil(radius * 1.1f);

        // ライトの仮のビュー行列を作成
        Vector3 up = { 0.0f, 1.0f, 0.0f };
        if (std::abs(normLightDir.y) > 0.99f) up = { 1.0f, 0.0f, 0.0f };

        // 中心点からライトの方向へ少し引いた位置を仮の光源位置とする
        Vector3 lightPos = center - (normLightDir * radius);
        Matrix4x4 lightView = Matrix4x4::MakeLookAt(lightPos, center, up);

        // テクセルスナップ
        // カメラが移動したときに、影の輪郭がテクセル単位でカチッと固定されるように位置を丸める
        Matrix4x4 shadowProj = Matrix4x4::MakeOrthographic(radius * 2.0f, radius * 2.0f, 0.0f, radius * 2.0f);
        Matrix4x4 shadowViewProj = lightView * shadowProj;

        // 原点(0,0,0)をライトのViewProj空間に変換
        Vector3 shadowOrigin = { 0.0f, 0.0f, 0.0f };
        shadowOrigin = shadowViewProj.TransformPoint(shadowOrigin);
        // テクセル単位にスケール
        shadowOrigin = shadowOrigin * (shadowMapResolution / 2.0f);

        // 小数点以下を丸める（スナップ）
        Vector3 roundedOrigin{ std::round(shadowOrigin.x), std::round(shadowOrigin.y), std::round(shadowOrigin.z) };
        Vector3 roundOffset = roundedOrigin - shadowOrigin;
        // 再び元のスケールに戻す
        roundOffset = roundOffset * (2.0f / shadowMapResolution);

        // 正射影行列のズレを補正する（スナップ処理）
        shadowProj.m[3][0] += roundOffset.x;
        shadowProj.m[3][1] += roundOffset.y;

        // 最終的な行列を確定させて保存
        shadowData_->cascadeLightViewProj[i] = lightView * shadowProj;
    }
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
        up = Math::CrossProduct(dir, right).Normalize();

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

void LightManager::DrawSelectedLightGizmo()
{
#ifdef IS_DEVELOPMENT
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
    else if (selectedLightType_ == SelectedLightType::Area) {
        AreaLight& light = areaLightData_[selectedLightIndex_];
        pos = light.position;
        // 右と上のベクトルから行列を構築
        Vector3 forward = Math::CrossProduct(light.right, light.up);
        lightMat = Matrix4x4::MakeFromAxes(light.right, light.up, forward, pos);
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
            light.position = outPos; // Translateの適用
            // Scaleの適用（X,Y,Zの平均値を半径にする）
            light.radius = std::max(0.1f, (outScale.x + outScale.y + outScale.z) / 3.0f);
        }
        else if (selectedLightType_ == SelectedLightType::Spot) {
            SpotLight& light = spotLightData_[selectedLightIndex_];
            light.position = outPos;

            // Rotateの適用：行列のZ軸成分（m[2][0], m[2][1], m[2][2]）が前方ベクトル(Direction)
            Vector3 newDir = { lightMat.m[2][0], lightMat.m[2][1], lightMat.m[2][2] };
            light.direction = newDir.Normalize();
        }
        else if (selectedLightType_ == SelectedLightType::Area) {
            AreaLight& light = areaLightData_[selectedLightIndex_];
            light.position = outPos;

            // Rotate/Scaleの適用：行列のX軸とY軸のベクトルをそのままRight/Upに使う
            light.right = { lightMat.m[0][0], lightMat.m[0][1], lightMat.m[0][2] };
            light.up = { lightMat.m[1][0], lightMat.m[1][1], lightMat.m[1][2] };
        }
        else if (selectedLightType_ == SelectedLightType::Directional) {
            DirectionalLight& light = directionalLightData_[selectedLightIndex_];

            // 仮想位置を更新
            directionalLightPositions_[selectedLightIndex_] = outPos;

            // 回転結果の行列のZ軸(Forward)から新しい向きを計算
            Vector3 newDir = { lightMat.m[2][0], lightMat.m[2][1], lightMat.m[2][2] };
            light.direction = newDir.Normalize();
        }
    }
#endif
}

}