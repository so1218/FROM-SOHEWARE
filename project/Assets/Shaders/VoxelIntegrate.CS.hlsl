#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelTemporalOut : register(t0);
RWTexture3D<float4> gVoxelAccumulate : register(u0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// スレッドはXとYにしか展開しない
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelTemporalOut.GetDimensions(width, height, depth);

    if (DTid.x >= width || DTid.y >= height)
        return;
    
    float3 volumetricIllumination = float3(0, 0, 0);
    float transmittance = 1.0f;

    // 手前から奥に向かって積分レイマーチング
    for (uint z = 0; z < depth; ++z)
    {
        uint3 voxelCoord = uint3(DTid.x, DTid.y, z);
        float4 stepData = gVoxelTemporalOut.Load(int4(voxelCoord, 0));
        
        float3 S = stepData.rgb; // 単位長さあたりの散乱光
        float extinction = max(stepData.a, 0.00001f); // 消散係数
        
        // 現在のボクセルステップ単体の透過率
        float stepTransmittance = exp(-extinction);
        
        // 散乱光(S)に対して、このボクセルステップ内でどれだけ光が残り、どれだけ消散したかを正しく乗算
        float3 integratedScattering = S * (1.0f - stepTransmittance) / extinction;
        
        // 全体の透過率を考慮して累積
        volumetricIllumination += integratedScattering * transmittance;
        
        // 次のステップへ透過率を更新
        transmittance *= stepTransmittance;

        // 最終結果を書き込み
        gVoxelAccumulate[voxelCoord] = float4(volumetricIllumination, transmittance);
    }
}