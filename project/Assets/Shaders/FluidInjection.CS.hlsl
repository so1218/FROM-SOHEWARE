#include "ShaderConstants.hlsli"

// 読み書き両用（Advectionが終わったあとの状態に対して、さらに力を加算する）
RWTexture3D<float4> gVelocity : register(u0);
RWTexture3D<float> gDensity : register(u1);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocity.GetDimensions(width, height, depth);

    // 範囲外アクセスを防止
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    // 1. ボクセルの空間座標を 0.0 ～ 1.0 の UVW に変換
    float3 uvw = (float3(DTid) + 0.5f) / float3(width, height, depth);

    // 2. UVW を使って、このボクセルの「ワールド座標」を復元する
    // これにより、プレイヤーのワールド座標と正確に距離を測ることができます
    float3 voxelWorldPos = lerp(gFluidSettings.gridMin, gFluidSettings.gridMax, uvw);

    // 3. プレイヤーとの距離を計算
    float dist = distance(voxelWorldPos, gFluidSettings.objectPos);

    // 4. 影響力（フォールオフ）の計算
    // プレイヤーの中心に近いほど 1.0 になり、interactionRadius で 0.0 になる滑らかな曲線
    float influence = smoothstep(gFluidSettings.interactionRadius, 0.0f, dist);

    // 影響範囲内にあるボクセルのみ、力を加算する
    if (influence > 0.0f)
    {
        // 現在の値を読み込み
        float3 currentVel = gVelocity[DTid.xyz].xyz;
        float currentDen = gDensity[DTid.xyz];

        // プレイヤーの速度に、影響力・全体の強さ・DeltaTime を掛けて「加算する風の力」を計算
        float3 addedVel = gFluidSettings.objectVelocity * influence * gFluidSettings.injectionStrength * gFrameData.deltaTime;
        
        // 密度を加算（オプション：例えば攻撃の軌跡に沿ってフワッと霧を出したい場合など）
        float addedDen = gFluidSettings.densityAmount * influence * gFrameData.deltaTime;

        // 加算してテクスチャに直接書き戻す
        gVelocity[DTid.xyz] = float4(currentVel + addedVel, 0.0f);
        gDensity[DTid.xyz] = currentDen + addedDen;
    }
}