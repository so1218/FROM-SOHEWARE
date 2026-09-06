#include "Common/ShaderConstants.hlsli"
#include "Common/CameraUtils.hlsli"

Texture2D<float> gDepthTexture : register(t0);
// TAAされて綺麗になった、蓄積済みの3Dフォグ
Texture3D<float4> gVoxelAccumulate : register(t1);
SamplerState gLinearSampler : register(s0);

RWTexture2D<float4> gOutput : register(u0);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// IGNノイズでZスライス境界のバンディングを無効化
float InterleavedGradientNoise(float2 pixelCoord)
{
    static const float3 kIGNMagic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(kIGNMagic.z * frac(dot(pixelCoord, kIGNMagic.xy)));
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutput.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;
    
    float2 texelSize = 1.0f / float2(width, height);
    float2 uv = (float2(DTid.xy) + 0.5f) * texelSize;
    
    float depthVal = gDepthTexture.SampleLevel(gLinearSampler, uv, 0).r;
    
    float4 clipPos = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, depthVal, 1.0f);
    float4 worldPosFull = mul(clipPos, gFrameData.invViewProj);
    float3 worldPos = worldPosFull.xyz / worldPosFull.w;

    float nearZ = max(gFrameData.nearClip, kMinNearClip);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);
    float rayLength = length(worldPos - gFrameData.cameraWorldPosition);
    float clampedDistance = clamp(rayLength, nearZ, farZ);
    
    // 指数スライス計算
    float zSliceContinuous = (log2(clampedDistance / nearZ) / log2(farZ / nearZ));
    
    // ジッターを少し乗せてFroxel格子のバンディング（縞模様）を滑らかに分散させる
    float jitter = (InterleavedGradientNoise(DTid.xy) - 0.5f) * 0.005f;
    float zSlice = saturate(zSliceContinuous + jitter);
    
    float3 sampleUVW = float3(uv, zSlice);
    float4 finalFog = gVoxelAccumulate.SampleLevel(gLinearSampler, sampleUVW, 0);
    
    gOutput[DTid.xy] = finalFog;
}