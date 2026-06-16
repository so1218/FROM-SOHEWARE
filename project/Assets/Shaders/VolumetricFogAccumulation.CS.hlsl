#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelFiltered : register(t0);

RWTexture3D<float4> gVoxelAccumulate : register(u0);

ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// スレッドはXとYにしか展開しない
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelFiltered.GetDimensions(width, height, depth);

    // 画面外なら終了
    if (DTid.x >= width || DTid.y >= height)
        return;
    
    float3 volumetricIllumination = float3(0, 0, 0);
    float transmittance = 1.0f;

    // 手前から奥に向かってレイマーチング
    for (uint z = 0; z < depth; ++z)
    {
        uint3 voxelCoord = uint3(DTid.x, DTid.y, z);
        float4 stepData = gVoxelFiltered.Load(int4(voxelCoord, 0));
        
        // Injectionパスで既に積分済みの散乱光
        float3 S = stepData.rgb;
        float extinction = stepData.a;
        
        // 透過率だけはここで計算
        float stepTransmittance = exp(-extinction);

        // 透過率を掛けて足す
        float3 stepScattering = S;
        
        volumetricIllumination += stepScattering * transmittance;
        transmittance *= stepTransmittance;

        gVoxelAccumulate[voxelCoord] = float4(volumetricIllumination, transmittance);
    }
}