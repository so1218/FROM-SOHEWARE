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

// 各ピクセルで、物までに積み上がった霧を読む
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint outputWidth, outputHeight;
    gOutput.GetDimensions(outputWidth, outputHeight);
    if (any(dispatchThreadId.xy >= uint2(outputWidth, outputHeight)))
        return;

    float2 screenUV = (float2(dispatchThreadId.xy) + 0.5f) / float2(outputWidth, outputHeight);

    // 補間すると輪郭で手前と奥の深度が混ざるので、補間せずに読む
    uint depthWidth, depthHeight;
    gDepthTexture.GetDimensions(depthWidth, depthHeight);
    int2 depthCoord = min(int2(screenUV * float2(depthWidth, depthHeight)), int2(depthWidth, depthHeight) - 1);
    float sceneDepth = gDepthTexture.Load(int3(depthCoord, 0));
    float4 sceneWorld = mul(float4(screenUV.x * 2.0f - 1.0f, (1.0f - screenUV.y) * 2.0f - 1.0f, sceneDepth, 1.0f), gFrameData.invViewProj);
    float sceneDistance = length(sceneWorld.xyz / sceneWorld.w - gFrameData.cameraWorldPosition);

    float nearDistance = GetFroxelNearDistance(gFrameData.nearClip);
    float farDistance = GetFroxelFarDistance(gFrameData.farClip, gFogSettings.maxDistance);
    float depthUV = FroxelDistanceToDepthUV(clamp(sceneDistance, nearDistance, farDistance), nearDistance, farDistance);

    // 各スライスにはスライスの奥の端までの霧が入っているので、半スライス手前を読む
    uint froxelWidth, froxelHeight, froxelDepth;
    gVoxelAccumulate.GetDimensions(froxelWidth, froxelHeight, froxelDepth);
    depthUV -= 0.5f / float(froxelDepth);

    // スライスの境目の縞を目立たなくする
    depthUV = saturate(depthUV + (InterleavedGradientNoise(dispatchThreadId.xy) - 0.5f) * 0.005f);

    gOutput[dispatchThreadId.xy] = gVoxelAccumulate.SampleLevel(gLinearSampler, float3(screenUV, depthUV), 0);
}
