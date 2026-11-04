#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"
#include "Volumetric/FroxelCommon.hlsli"

Texture3D<float4> gVoxelTemporalOut : register(t0);
RWTexture3D<float4> gVoxelAccumulate : register(u0);
ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// 手前のスライスから順に積み上げて、各スライスに「そこまでの光」と「そこまでの透過率」を書く
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint froxelWidth, froxelHeight, froxelDepth;
    gVoxelTemporalOut.GetDimensions(froxelWidth, froxelHeight, froxelDepth);
    if (any(dispatchThreadId.xy >= uint2(froxelWidth, froxelHeight)))
        return;

    float nearDistance = GetFroxelNearDistance(gFrameData.nearClip);
    float farDistance = GetFroxelFarDistance(gFrameData.farClip, gFogSettings.maxDistance);

    // スライスの境目は等比で並ぶので、次の境目は掛け算で出せる
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

        // スライスの中で光が減っていく分も考えて積分する (Frostbite の方法)
        accumulatedLight += scattering * (1.0f - sliceTransmittance) / extinction * transmittance;
        transmittance *= sliceTransmittance;

        gVoxelAccumulate[froxelCoord] = float4(accumulatedLight, transmittance);
    }
}
