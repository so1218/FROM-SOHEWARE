#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

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

    float2 uv = (pixelPos + 0.5f) / float2(width, height);
    float2 worldXZ = gInteractionConstants.centerWorldPos + (uv - 0.5f) * gInteractionConstants.worldSize;

    float2 terrainUV = CalculateTerrainUV(worldXZ);
    float rawHeight = gTerrainHeightMap.SampleLevel(gLinearSampler, terrainUV, 0).r;
    float terrainHeight = (rawHeight - 0.5f) * gInteractionConstants.terrainHeightScale;

    // 前フレームの軌跡サンプリング
    float2 worldDelta = gInteractionConstants.centerWorldPos - gInteractionConstants.prevCenterWorldPos;
    float2 prevUV = uv + (worldDelta / gInteractionConstants.worldSize);

    float fadedTrail = 0.0f;
    if (all(prevUV >= 0.0f) && all(prevUV <= 1.0f))
    {
        float4 prevData = gPrevInteractionMap.SampleLevel(gPointSampler, prevUV, 0);
        fadedTrail = max(0.0f, prevData.a - (gFrameData.deltaTime / gInteractionConstants.trailDuration));
        
        // 境界付近でフェードアウトさせ、外に出た際のカクつきを防止
        float2 edgeFade = smoothstep(0.0f, 0.05f, prevUV) * smoothstep(1.0f, 0.95f, prevUV);
        fadedTrail *= (edgeFade.x * edgeFade.y);
    }

    float maxCurrentStrength = 0.0f;
    float2 totalPushDir = float2(0.0f, 0.0f);

    // エンティティ干渉
    for (uint i = 0; i < gInteractionConstants.entityCount; ++i)
    {
        InteractionEntity entity = gEntities[i];

        float yDiff = abs(entity.position.y - terrainHeight);
        if (yDiff > entity.maxVerticalDist)
            continue;

        float2 diffXZ = worldXZ - entity.position.xz;
        float distXZ = length(diffXZ);

        if (distXZ < entity.radius)
        {
            float verticalFactor = smoothstep(1.0f, 0.0f, saturate(yDiff / entity.maxVerticalDist));
            float horizontalFactor = smoothstep(1.0f, 0.0f, saturate(distXZ / entity.radius));
            float strength = horizontalFactor * verticalFactor * entity.forceMultiplier;

            maxCurrentStrength = max(maxCurrentStrength, strength);
            
            float2 radialDir = (distXZ > kEpsilon) ? (diffXZ / distXZ) : float2(0.0f, 1.0f);
            float2 entityPushDir = float2(0.0f, 0.0f);

            if (entity.entityType == 2) // 爆発・衝撃波
            {
                entityPushDir = radialDir * strength * 2.0f;
            }
            else // キャラクター・車両
            {
                float speedSq = dot(entity.velocity.xz, entity.velocity.xz);
                float2 moveDir = (speedSq > 0.01f) ? normalize(entity.velocity.xz) : radialDir;

                // 進行方向と放射方向をブレンドし、引き波を形成
                float2 wakeDir = normalize(moveDir * 0.4f + radialDir * 0.6f);
                entityPushDir = wakeDir * strength;
            }

            totalPushDir += entityPushDir;
        }
    }

    // デコードと出力
    float dirSq = dot(totalPushDir, totalPushDir);
    float2 finalPushDir = (dirSq > kEpsilon) ? (totalPushDir * rsqrt(dirSq)) : float2(0.0f, 0.0f);
    float2 encodedDir = finalPushDir * 0.5f + 0.5f;

    gOutputInteractionMap[pixelPos] = float4(encodedDir, maxCurrentStrength, max(maxCurrentStrength, fadedTrail));
}