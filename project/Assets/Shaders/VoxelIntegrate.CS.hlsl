#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelTemporalOut : register(t0);
RWTexture3D<float4> gVoxelAccumulate : register(u0);
ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

static const float MIN_NEAR_Z = 0.1f; // 指数深度計算のためのニアクリップ下限

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelTemporalOut.GetDimensions(width, height, depth);

    if (DTid.x >= width || DTid.y >= height)
        return;
    
    float3 volumetricIllumination = float3(0, 0, 0);
    float transmittance = 1.0f;

    float nearZ = max(gFrameData.nearClip, MIN_NEAR_Z);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);

    // Zスライスの等比倍率をループ外で事前計算
    float sliceRatio = pow(farZ / nearZ, 1.0f / float(depth));
    
    // 最初のスライスの深度
    float currentViewZ = nearZ;

    // 手前から奥に向かって積分レイマーチング
    for (uint z = 0; z < depth; ++z)
    {
        uint3 voxelCoord = uint3(DTid.x, DTid.y, z);
        float4 stepData = gVoxelTemporalOut.Load(int4(voxelCoord, 0));
        
        float3 S = stepData.rgb;
        float extinction = max(stepData.a, EXTINCTION_EPSILON);
        
        // 現在のボクセルの厚みを計算し、次の深度を更新 
        float nextViewZ = currentViewZ * sliceRatio;
        float voxelThickness = nextViewZ - currentViewZ;
        currentViewZ = nextViewZ;

        // ボクセル内の解析的積分
        float stepTransmittance = exp(-extinction * voxelThickness);
        float3 integratedScattering = S * (1.0f - stepTransmittance) / extinction;
        
        // 全体の透過率を考慮して累積
        volumetricIllumination += integratedScattering * transmittance;
        transmittance *= stepTransmittance;

        // 結果を書き込み
        gVoxelAccumulate[voxelCoord] = float4(volumetricIllumination, transmittance);
    }
}