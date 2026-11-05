#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"
#include "Volumetric/FroxelCommon.hlsli"

Texture3D<float4> gVoxelTemporalOut : register(t0);
RWTexture3D<float4> gVoxelAccumulate : register(u0);
ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// 手前のスライスから順に、そこまでの光と透過率を積み上げる
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint froxelWidth, froxelHeight, froxelDepth;
    gVoxelTemporalOut.GetDimensions(froxelWidth, froxelHeight, froxelDepth);
    if (any(dispatchThreadId.xy >= uint2(froxelWidth, froxelHeight)))
        return;

    float nearDistance = GetFroxelNearDistance(gFrameData.nearClip);
    float farDistance = GetFroxelFarDistance(gFrameData.farClip, gFogSettings.maxDistance);

    // 境目は等比で並ぶので、掛け算で次へ進める
    float sliceRatio = pow(farDistance / nearDistance, 1.0f / float(froxelDepth));
    float sliceStart = nearDistance;

    float3 accumulatedLight = float3(0.0f, 0.0f, 0.0f);
    float transmittance = 1.0f;

    for (uint z = 0; z < froxelDepth; ++z)
    {
        uint3 froxelCoord = uint3(dispatchThreadId.xy, z);
        float4 media = gVoxelTemporalOut.Load(int4(froxelCoord, 0));
        float3 scattering = media.rgb;
        float extinction = max(media.a, kEpsilon);

        float sliceEnd = sliceStart * sliceRatio;
        float sliceTransmittance = exp(-extinction * (sliceEnd - sliceStart));
        sliceStart = sliceEnd;

        // スライスの中で減っていく分も含めて積分する。Frostbite の式
        accumulatedLight += scattering * (1.0f - sliceTransmittance) / extinction * transmittance;
        transmittance *= sliceTransmittance;

        gVoxelAccumulate[froxelCoord] = float4(accumulatedLight, transmittance);
    }
}
