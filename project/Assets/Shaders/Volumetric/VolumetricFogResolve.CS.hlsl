#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"
#include "Volumetric/FroxelCommon.hlsli"

Texture2D<float> gDepthTexture : register(t0);
Texture3D<float4> gVoxelAccumulate : register(t1);
SamplerState gLinearSampler : register(s0);

// rgb: 霧の光, a: 奥の景色がどれだけ残るか
RWTexture2D<float4> gOutput : register(u0);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// 画面の各ピクセルで、手前の物までに積み上がった霧を読む
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint outputWidth, outputHeight;
    gOutput.GetDimensions(outputWidth, outputHeight);
    if (any(dispatchThreadId.xy >= uint2(outputWidth, outputHeight)))
        return;

    float2 screenUV = (float2(dispatchThreadId.xy) + 0.5f) / float2(outputWidth, outputHeight);

    float sceneDepth = gDepthTexture.SampleLevel(gLinearSampler, screenUV, 0).r;
    float4 sceneWorld = mul(float4(screenUV.x * 2.0f - 1.0f, (1.0f - screenUV.y) * 2.0f - 1.0f, sceneDepth, 1.0f), gFrameData.invViewProj);
    float sceneDistance = length(sceneWorld.xyz / sceneWorld.w - gFrameData.cameraWorldPosition);

    float nearDistance = GetFroxelNearDistance(gFrameData.nearClip);
    float farDistance = GetFroxelFarDistance(gFrameData.farClip, gFogSettings.maxDistance);
    float depthUV = FroxelDistanceToDepthUV(clamp(sceneDistance, nearDistance, farDistance), nearDistance, farDistance);

    // 積み上げたテクスチャの各スライスには「そのスライスの奥の端まで」の霧が入っているので、
    // 物の位置ちょうどの値を読むには半スライス手前にずらす
    uint froxelWidth, froxelHeight, froxelDepth;
    gVoxelAccumulate.GetDimensions(froxelWidth, froxelHeight, froxelDepth);
    depthUV -= 0.5f / float(froxelDepth);

    // スライスの境目が縞に見えないように、少しだけ散らす
    depthUV = saturate(depthUV + (InterleavedGradientNoise(dispatchThreadId.xy) - 0.5f) * 0.005f);

    gOutput[dispatchThreadId.xy] = gVoxelAccumulate.SampleLevel(gLinearSampler, float3(screenUV, depthUV), 0);
}
