#include "ShaderConstants.hlsli"

Texture2D<float> gDepthTexture : register(t0);
// 既にTAAされて綺麗になった、蓄積済みの3Dフォグ
Texture3D<float4> gVoxelAccumulate : register(t1);
SamplerState gLinearSampler : register(s0);

RWTexture2D<float4> gOutput : register(u0);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutput.GetDimensions(width, height);
    
    if (DTid.x >= width || DTid.y >= height)
        return;

    float2 texelSize = 1.0f / float2(width, height);
    float2 uv = (float2(DTid.xy) + 0.5f) * texelSize;
    
    // 1. 不透明オブジェクトの深度を取得
    float depthVal = gDepthTexture.SampleLevel(gLinearSampler, uv, 0).r;
    
    // 2. ワールド座標を復元し、カメラからの距離を計算
    float4 clipPos = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, depthVal, 1.0f);
    float4 worldPosFull = mul(clipPos, gFrameData.invViewProj);
    float3 worldPos = worldPosFull.xyz / worldPosFull.w;

    float nearZ = max(gFrameData.nearClip, 0.1f);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);
    float rayLength = length(worldPos - gFrameData.cameraWorldPosition);
    float clampedDistance = clamp(rayLength, nearZ, farZ);
    
    // 3. 距離から3DテクスチャのZスライス（深度UV）を計算
    float zSlice = log2(clampedDistance / nearZ) / log2(farZ / nearZ);
    zSlice = saturate(zSlice);
    
    // 4. 完成した3Dフォグをサンプリング
    float3 sampleUVW = float3(uv, zSlice);
    float4 finalFog = gVoxelAccumulate.SampleLevel(gLinearSampler, sampleUVW, 0);
    
    // 出力
    gOutput[DTid.xy] = finalFog;
}