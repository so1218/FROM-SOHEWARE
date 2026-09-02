#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<InteractionConstants> gInteractionConstants : register(b1);
StructuredBuffer<InteractionEntity> gEntities : register(t0);
Texture2D<float> gTerrainHeightMap : register(t1);
Texture2D<float4> gPrevInteractionMap : register(t2);
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler : register(s1);

RWTexture2D<float4> gOutputInteractionMap : register(u0);

float2 CalculateTerrainUV(float2 worldXZ)
{
    float u = (worldXZ.x - gInteractionConstants.terrainCenter.x) / gInteractionConstants.terrainSize.x + 0.5f;
    float v = (worldXZ.y - gInteractionConstants.terrainCenter.y) / gInteractionConstants.terrainSize.y + 0.5f;
    return float2(u, v);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelPos = dispatchThreadID.xy;
    uint width, height;
    gOutputInteractionMap.GetDimensions(width, height);
    if (pixelPos.x >= width || pixelPos.y >= height)
        return;

    // カレントピクセルのワールドXZ座標の算出
    float2 uv = (pixelPos + 0.5f) / float2(width, height);
    float2 worldXZ = gInteractionConstants.centerWorldPos + (uv - 0.5f) * gInteractionConstants.worldSize;

    // ハイトマップUVと高さを正確に取得
    float2 terrainUV = CalculateTerrainUV(worldXZ);
    
    // UVが領域外の場合は高さを0（または範囲外処理）
    float rawHeight = gTerrainHeightMap.SampleLevel(gLinearSampler, terrainUV, 0).r;
    float terrainHeight = (rawHeight - 0.5f) * gInteractionConstants.terrainHeightScale;

    // 前フレーム足跡のサンプリング
    float2 worldDelta = gInteractionConstants.centerWorldPos - gInteractionConstants.prevCenterWorldPos;
    float2 prevUV = uv + (worldDelta / gInteractionConstants.worldSize);

    float fadedTrail = 0.0f;
    if (all(prevUV >= 0.0f) && all(prevUV <= 1.0f))
    {
        // バイリニアブラーによる急速消滅を防ぐため gPointSampler を使用
        float4 prevData = gPrevInteractionMap.SampleLevel(gPointSampler, prevUV, 0);
        fadedTrail = max(0.0f, prevData.a - (gFrameData.deltaTime / gInteractionConstants.trailDuration));
    }

    float maxCurrentStrength = 0.0f;
    float2 totalPushDir = float2(0, 0);

    // 4. エリア内の全エンティティとの干渉計算
    for (uint i = 0; i < gInteractionConstants.entityCount; ++i)
    {
        InteractionEntity entity = gEntities[i];

        // Y軸（高さ）判定
        float yDiff = abs(entity.position.y - terrainHeight);
        if (yDiff > entity.maxVerticalDist)
            continue;

        float verticalFactor = smoothstep(0.0f, 1.0f, 1.0f - saturate(yDiff / entity.maxVerticalDist));

        // XZ軸（平面距離）判定
        float2 diffXZ = worldXZ - entity.position.xz;
        float distXZ = length(diffXZ);

        if (distXZ < entity.radius)
        {
            float horizontalFactor = smoothstep(0.0f, 1.0f, 1.0f - saturate(distXZ / entity.radius));
            float strength = horizontalFactor * verticalFactor * entity.forceMultiplier;

            if (strength > maxCurrentStrength)
            {
                maxCurrentStrength = strength;
            }

            // entityType に応じた方向・挙動の分岐
            float2 entityPushDir = float2(0, 0);
            float2 radialDir = (distXZ > 0.001f) ? normalize(diffXZ) : float2(0, 1);

            if (entity.entityType == 2) // 衝撃波・爆発
            {
                // 進行方向無視で純粋な強い放射状の力
                entityPushDir = radialDir * strength * 2.0f;
            }
            else // 人間 (0) または 大型/車両 (1)
            {
                float speed = length(entity.velocity.xz);
                float2 moveDir = (speed > 0.1f) ? normalize(entity.velocity.xz) : radialDir;

                // 高速移動時はV字型の引き波 (Wake)を作るため斜め後ろに拡散
                float2 wakeDir = normalize(moveDir * 0.4f + radialDir * 0.6f);
                entityPushDir = wakeDir * strength;
            }

            totalPushDir += entityPushDir;
        }
    }

    // 方向ベクトルの正規化と 0.0~1.0 エンコード (-1~1 -> 0~1)
    float pushLen = length(totalPushDir);
    float2 finalPushDir = (pushLen > 0.001f) ? (totalPushDir / pushLen) : float2(0, 0);
    float2 encodedDir = finalPushDir * 0.5f + 0.5f;

    // Aチャンネル: 痕跡の保持
    float newTrail = max(maxCurrentStrength, fadedTrail);

    gOutputInteractionMap[pixelPos] = float4(encodedDir, maxCurrentStrength, newTrail);
}