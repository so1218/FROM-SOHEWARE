#include "ShaderConstants.hlsli"

// 読み込み用（前のフレームの流体状態）
Texture3D<float4> gVelocityRead : register(t0); // xyz: 風の速度
Texture3D<float> gDensityRead : register(t1); // r: 霧の密度

// サンプラー（※必ず「Clamp」設定のバイリニア/トリリニアサンプラーを使用すること）
SamplerState gLinearClampSampler : register(s0);

// 書き込み用（今のフレームの新しい流体状態）
RWTexture3D<float4> gVelocityWrite : register(u0);
RWTexture3D<float> gDensityWrite : register(u1);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocityWrite.GetDimensions(width, height, depth);

    // 範囲外アクセスを防止
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    // 1. 現在のボクセル座標を 0.0 ～ 1.0 の UVW 空間に変換
    // ピクセルの中央をサンプリングするために +0.5f します
    float3 uvw = (float3(DTid) + 0.5f) / float3(width, height, depth);

    // 2. 現在のボクセルに吹いている「風の速度」を取得
    // 速度が整数で書き込まれている場合を考慮し、UVW座標で取得
    float3 currentVelocity = gVelocityRead.SampleLevel(gLinearClampSampler, uvw, 0).xyz;

    // 3. セミ・ラグランジュ法による「過去の座標」の計算
    // 現在の座標から、風の逆方向に時間を遡ります
    // gridScale は空間の解像度と物理的な移動量を合わせるための係数です
    float3 backtraceUVW = uvw - (currentVelocity * gFrameData.deltaTime * gFluidSettings.gridScale);

    // 4. 過去の座標から、密度と速度をサンプリング（トリリニア補間）
    float3 advectedVelocity = gVelocityRead.SampleLevel(gLinearClampSampler, backtraceUVW, 0).xyz;
    float advectedDensity = gDensityRead.SampleLevel(gLinearClampSampler, backtraceUVW, 0).r;

    // 5. 減衰（Dissipation）の適用
    // 永遠に動き続けたり、霧が残り続けたりしないように少しずつ減衰させます
    advectedVelocity *= gFluidSettings.velocityDissipation;
    advectedDensity *= gFluidSettings.densityDissipation;

    // 6. 新しいバッファに書き込み
    gVelocityWrite[DTid.xyz] = float4(advectedVelocity, 0.0f);
    gDensityWrite[DTid.xyz] = advectedDensity;
}