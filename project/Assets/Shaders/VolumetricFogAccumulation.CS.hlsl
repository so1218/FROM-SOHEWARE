#include "ShaderConstants.hlsli"

// --- 入出力リソース ---
// 第1パス(Injection)で作った「各ボクセル単体の光と密度」
Texture3D<float4> gVoxelFiltered : register(t0);

// 今回の出力先。手前から奥へ蓄積された「最終的な光と透過率」
RWTexture3D<float4> gVoxelAccumulate : register(u0);

ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// 【ポイント】スレッドはXとY（2D画面）にしか展開しません！Zはループで処理します。
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelFiltered.GetDimensions(width, height, depth);

    // 画面外なら終了
    if (DTid.x >= width || DTid.y >= height)
        return;

    // --- あなたの元のコードと同じ初期化 ---
    float3 volumetricIllumination = float3(0, 0, 0);
    float transmittance = 1.0f;

    // 手前（Z = 0）から奥（Z = depth - 1）に向かってレイマーチング（ボクセルを辿る）
    for (uint z = 0; z < depth; ++z)
    {
        uint3 voxelCoord = uint3(DTid.x, DTid.y, z);
        float4 stepData = gVoxelFiltered.Load(int4(voxelCoord, 0));
        
        // Injectionパスで既に積分済みの散乱光
        float3 S = stepData.rgb;
        float extinction = stepData.a;
        
        // 透過率だけはここで計算（奥へ行くほど光が遮られるため）
        float stepTransmittance = exp(-extinction);

        // 【修正】二重積分をやめ、単に透過率を掛けて足すだけにする
        float3 stepScattering = S;
        
        volumetricIllumination += stepScattering * transmittance;
        transmittance *= stepTransmittance;

        gVoxelAccumulate[voxelCoord] = float4(volumetricIllumination, transmittance);
    }
}