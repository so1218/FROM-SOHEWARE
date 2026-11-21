#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<GrassFireConstants> gFire : register(b0);
ConstantBuffer<GlobalEnvironmentData> gEnvironment : register(b1);

Texture2D<float4> gPreviousFireMap : register(t0);
Texture2D<float> gDensityMap : register(t1);
SamplerState gLinearSampler : register(s0);

// r: 炎の強さ, g: 燃え進み, b: たまった熱, a: ここからあと何 m 燃え広がれるか
RWTexture2D<float4> gFireMap : register(u0);

// 燃え広がる距離を決めないときの、実質どこまでも広がる距離
static const float kUnlimitedSpreadDistance = 60000.0f;
// 燃え広がれる距離が残りこれより少ないと、熱を受けにくくして火の縁を自然に止める
static const float kSpreadFadeDistance = 3.0f;

// 燃え始めにすぐ強くなり、燃え尽きる前にゆっくり弱まる
float CalculateFireIntensity(float burnProgress)
{
    if (burnProgress <= 0.0f || burnProgress >= 1.0f)
        return 0.0f;
    return smoothstep(0.0f, 0.08f, burnProgress) * (1.0f - smoothstep(0.45f, 1.0f, burnProgress));
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint width, height;
    gFireMap.GetDimensions(width, height);
    if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
        return;

    int2 texel = int2(dispatchThreadId.xy);
    if (gFire.resetRequested != 0)
    {
        gFireMap[texel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    float4 previous = gPreviousFireMap.Load(int3(texel, 0));
    float burnProgress = previous.g;
    float heat = previous.b;
    float spreadBudget = previous.a;

    float2 fireUV = (float2(texel) + 0.5f) / float2(width, height);
    float2 worldXZ = gFire.areaCenter + (fireUV - 0.5f) * gFire.areaSize;

    // 草が生えていない所は燃えず、そこで火が止まる
    bool hasFuel = gDensityMap.SampleLevel(gLinearSampler, fireUV, 0) >= gFire.densityThreshold;

    // マスごとに燃えやすさと燃える長さを変え、燃え広がる前線を不揃いにする
    float cellRandom = Hash12(float2(texel) * 0.731f + 17.0f);
    float ignitionThreshold = lerp(0.6f, 1.4f, cellRandom);

    if (burnProgress <= 0.0f)
    {
        if (hasFuel)
        {
            // 周りの 8 マスの炎から熱を受ける
            float2 windDirection = normalize(gEnvironment.windDirection + float2(0.00001f, 0.0f));
            float windAmount = gEnvironment.windSpeed * gFire.windSpreadBias;
            int2 maxTexel = int2(width, height) - 1;
            float2 cellSize = gFire.areaSize / float2(width, height);

            float receivedHeat = 0.0f;
            float bestBudget = 0.0f;
            [unroll]
            for (int offsetY = -1; offsetY <= 1; ++offsetY)
            {
                [unroll]
                for (int offsetX = -1; offsetX <= 1; ++offsetX)
                {
                    if (offsetX == 0 && offsetY == 0)
                        continue;

                    float4 neighbor = gPreviousFireMap.Load(int3(clamp(texel + int2(offsetX, offsetY), 0, maxTexel), 0));
                    float2 fromNeighbor = normalize(float2(-offsetX, -offsetY));
                    float windFactor = max(1.0f + windAmount * dot(fromNeighbor, windDirection), 0.05f);
                    float distanceWeight = (offsetX != 0 && offsetY != 0) ? 0.7071f : 1.0f;

                    // 隣から燃え移ると、その分だけ燃え広がれる距離が減る
                    float stepCost = length(float2(offsetX, offsetY) * cellSize) / windFactor;
                    float remainingBudget = neighbor.a - stepCost;
                    if (neighbor.r <= 0.0f || remainingBudget <= 0.0f)
                        continue;

                    float edgeFade = saturate(remainingBudget / kSpreadFadeDistance);
                    receivedHeat += neighbor.r * windFactor * distanceWeight * edgeFade;
                    bestBudget = max(bestBudget, remainingBudget);
                }
            }

            // 雨で濡れていると燃え広がりにくい
            receivedHeat *= gFire.spreadRate * (1.0f - saturate(gEnvironment.wetness) * gFire.rainSuppression);

            // 火が来なくなった所は少しずつ冷める
            heat = (receivedHeat > 0.0f) ? heat + receivedHeat * gFire.deltaTime : max(heat - gFire.coolingRate * gFire.deltaTime, 0.0f);
            spreadBudget = bestBudget;
        }

        // 火種
        for (uint ignitionIndex = 0; ignitionIndex < gFire.ignitionCount; ++ignitionIndex)
        {
            GrassFireIgnition ignition = gFire.ignitions[ignitionIndex];
            float distanceToIgnition = distance(worldXZ, ignition.position);
            float falloff = saturate((ignition.radius - distanceToIgnition) / max(ignition.radius * 0.5f, 0.001f));
            heat = max(heat, ignitionThreshold * ignition.strength * falloff);

            // 火種からは、決めた距離まで燃え広がれる
            if (falloff > 0.0f)
            {
                spreadBudget = (gFire.maxSpreadDistance > 0.0f) ? gFire.maxSpreadDistance : kUnlimitedSpreadDistance;
            }
        }

        if (hasFuel && heat >= ignitionThreshold)
        {
            burnProgress = 0.001f;
            heat = 0.0f;
        }
    }
    else if (burnProgress < 1.0f)
    {
        float burnDuration = max(gFire.burnDuration * lerp(0.75f, 1.25f, cellRandom), 0.01f);
        burnProgress = min(burnProgress + gFire.deltaTime / burnDuration, 1.0f);
    }

    // まだ燃えていない所は、隣から受けた燃え広がれる距離を次のフレームに持ち越さない
    float storedBudget = (burnProgress > 0.0f) ? spreadBudget : 0.0f;
    gFireMap[texel] = float4(CalculateFireIntensity(burnProgress), burnProgress, heat, storedBudget);
}
