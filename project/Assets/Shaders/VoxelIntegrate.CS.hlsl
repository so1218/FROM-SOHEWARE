#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelTemporalOut : register(t0);
RWTexture3D<float4> gVoxelAccumulate : register(u0);
ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelTemporalOut.GetDimensions(width, height, depth);

    if (DTid.x >= width || DTid.y >= height)
        return;
    
    float3 volumetricIllumination = float3(0, 0, 0);
    float transmittance = 1.0f;

    // 定数バッファからカメラのニア・ファークリップを取得
    float nearZ = max(gFrameData.nearClip, 0.1f);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);

    // 手前から奥に向かって積分レイマーチング
    for (uint z = 0; z < depth; ++z)
    {
        uint3 voxelCoord = uint3(DTid.x, DTid.y, z);
        float4 stepData = gVoxelTemporalOut.Load(int4(voxelCoord, 0));
        
        // 注入フェーズで厚みを掛けずに保存した単位長さあたりの純粋な値
        float3 S = stepData.rgb;
        float extinction = max(stepData.a, 0.00001f);
        
        // ここで現在のスライス（z）の「物理的な厚み」を都度計算する
        float zSlice0 = float(z) / float(depth);
        float zSlice1 = float(z + 1.0f) / float(depth);
        float viewZ0 = nearZ * pow(farZ / nearZ, zSlice0);
        float viewZ1 = nearZ * pow(farZ / nearZ, zSlice1);
        float voxelThickness = viewZ1 - viewZ0;

        // 消散係数にボクセルの厚みを掛けて、このステップの正確な透過率を出す
        float stepTransmittance = exp(-extinction * voxelThickness);
        
        // 散乱光(S)に対して、このボクセルステップ内でどれだけ光が残り、どれだけ消散したかを正しく乗算
        // （※数学的に積分を解くと、S側にvoxelThicknessを掛ける必要はなく、この式のままで完璧に成立します）
        float3 integratedScattering = S * (1.0f - stepTransmittance) / extinction;
        
        // 全体の透過率を考慮して累積
        volumetricIllumination += integratedScattering * transmittance;
        
        // 次のステップへ透過率を更新
        transmittance *= stepTransmittance;

        // 最終結果を書き込み
        gVoxelAccumulate[voxelCoord] = float4(volumetricIllumination, transmittance);
    }
}