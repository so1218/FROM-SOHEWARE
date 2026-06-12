#include "ShaderConstants.hlsli"

// 読み込み用（前のフレームの流体状態）
Texture3D<float4> gVelocityRead : register(t0); // xyz: 風の速度
Texture3D<float> gDensityRead : register(t1); // r: 霧の密度
Texture3D<float4> gUVWRead : register(t2);

// サンプラー（※必ず「Clamp」設定のバイリニア/トリリニアサンプラーを使用すること）
SamplerState gLinearClampSampler : register(s0);

// 書き込み用（今のフレームの新しい流体状態）
RWTexture3D<float4> gVelocityWrite : register(u0);
RWTexture3D<float> gDensityWrite : register(u1);
RWTexture3D<float4> gUVWWrite : register(u2);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocityWrite.GetDimensions(width, height, depth);

    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    float3 uvw = (float3(DTid) + 0.5f) / float3(width, height, depth);
    float3 currentVelocity = gVelocityRead.SampleLevel(gLinearClampSampler, uvw, 0).xyz;

    // セミ・ラグランジュ法
    float3 backtraceUVW = uvw - (currentVelocity * gFrameData.deltaTime * gFluidSettings.gridScale);

    // 速度と密度の移流
    float3 advectedVelocity = gVelocityRead.SampleLevel(gLinearClampSampler, backtraceUVW, 0).xyz;
    float advectedDensity = gDensityRead.SampleLevel(gLinearClampSampler, backtraceUVW, 0).r;
    
    // =======================================================
    // ★大改造1: UVW座標自体の移流
    // =======================================================
    // 過去の位置のUVW座標を取得する
    float3 advectedUVW = gUVWRead.SampleLevel(gLinearClampSampler, backtraceUVW, 0).xyz;

    // 【重要】無限に引き伸ばされる（ストレッチ）のを防ぐための緩和処理
    // わずかに元のグリッド座標(uvw)に戻すことで、ノイズの破綻を防ぎます。
    // gFluidSettings.uvwRelaxation は 0.1f ～ 0.5f 程度で調整。
    advectedUVW = lerp(advectedUVW, uvw, gFluidSettings.uvwRelaxation * gFrameData.deltaTime);

    advectedVelocity *= gFluidSettings.velocityDissipation;

    gVelocityWrite[DTid.xyz] = float4(advectedVelocity, 0.0f);
    gDensityWrite[DTid.xyz] = advectedDensity;
    
    // 移流したUVWを書き込む
    gUVWWrite[DTid.xyz] = float4(advectedUVW, 0.0f);
}