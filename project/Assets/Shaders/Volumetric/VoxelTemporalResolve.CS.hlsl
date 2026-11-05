#include "Common/ShaderConstants.hlsli"
#include "Volumetric/FroxelCommon.hlsli"

Texture3D<float4> gVoxelInjectFiltered : register(t0);
Texture3D<float4> gVoxelHistory : register(t1);
RWTexture3D<float4> gVoxelTemporalOut : register(u0);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

SamplerState gLinearClampSampler : register(s0);

// 急に変わったときに、今のフレームを混ぜる割合の上限
static const float kMaxCurrentWeight = 0.4f;
// この色の差で上限に達する
static const float kColorDifferenceForMaxWeight = 2.0f;
// 履歴を周りのばらつきの何倍まで許すか。毎フレームのずらしによるゆれは残す
static const float kClipStandardDeviationScale = 2.2f;

// 前のフレームの結果を、今の位置に合わせて読んで混ぜる
[numthreads(8, 8, 4)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint froxelWidth, froxelHeight, froxelDepth;
    gVoxelInjectFiltered.GetDimensions(froxelWidth, froxelHeight, froxelDepth);
    if (any(dispatchThreadId >= uint3(froxelWidth, froxelHeight, froxelDepth)))
        return;

    int3 maxCoord = int3(froxelWidth, froxelHeight, froxelDepth) - 1;
    float4 current = gVoxelInjectFiltered.Load(int4(dispatchThreadId, 0));

    // 周り 3x3x3 の平均とばらつきから外れた履歴は、古いとみなして切り詰める
    float4 sum = 0.0f;
    float4 sumSquared = 0.0f;
    for (int z = -1; z <= 1; ++z)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                int3 neighborCoord = clamp(int3(dispatchThreadId) + int3(x, y, z), 0, maxCoord);
                float4 neighbor = gVoxelInjectFiltered.Load(int4(neighborCoord, 0));
                sum += neighbor;
                sumSquared += neighbor * neighbor;
            }
        }
    }
    float4 mean = sum / 27.0f;
    float4 standardDeviation = sqrt(max(sumSquared / 27.0f - mean * mean, 0.0f));
    float4 clipMin = mean - kClipStandardDeviationScale * standardDeviation;
    float4 clipMax = mean + kClipStandardDeviationScale * standardDeviation;

    // このフロクセルの中心が、前のフレームではどこにあったか
    float nearDistance = GetFroxelNearDistance(gFrameData.nearClip);
    float farDistance = GetFroxelFarDistance(gFrameData.farClip, gFogSettings.maxDistance);

    float2 screenUV = (float2(dispatchThreadId.xy) + 0.5f) / float2(froxelWidth, froxelHeight);
    float distanceFromCamera = FroxelSliceToDistance(dispatchThreadId.z + 0.5f, froxelDepth, nearDistance, farDistance);
    float3 worldPosition = gFrameData.cameraWorldPosition
                         + GetViewRayDirection(screenUV, gFrameData.invViewProj, gFrameData.cameraWorldPosition) * distanceFromCamera;

    float4 previousClip = mul(float4(worldPosition, 1.0f), gFrameData.prevViewProj);
    float2 previousUV = previousClip.xy / previousClip.w * float2(0.5f, -0.5f) + 0.5f;
    float previousDistance = length(worldPosition - gFrameData.prevCameraWorldPosition);
    float3 previousUVW = float3(previousUV, FroxelDistanceToDepthUV(previousDistance, nearDistance, farDistance));

    float4 history = current;
    if (all(previousUVW >= 0.0f) && all(previousUVW <= 1.0f))
    {
        history = gVoxelHistory.SampleLevel(gLinearClampSampler, previousUVW, 0);
    }
    history = clamp(history, clipMin, clipMax);

    // 光が急に変わったときは、今のフレームを多めに混ぜて残像を減らす
    float colorDifference = length(current.rgb - history.rgb);
    float currentWeight = lerp(gFogSettings.temporalWeight, kMaxCurrentWeight, saturate(colorDifference / kColorDifferenceForMaxWeight));

    gVoxelTemporalOut[dispatchThreadId] = lerp(history, current, currentWeight);
}
