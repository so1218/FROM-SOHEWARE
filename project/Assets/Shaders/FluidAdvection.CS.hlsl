#include "ShaderConstants.hlsli"

Texture3D<float4> gVelocityRead : register(t0); // xyz: 風の速度
Texture3D<float> gDensityRead : register(t1); // r: 霧の密度
Texture3D<float4> gUVWRead : register(t2);

SamplerState gLinearClampSampler : register(s0);
SamplerState gLinearWrapSampler : register(s1);

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

    // テクスチャ内のローカルUV
    float3 uvw = (float3(DTid) + 0.5f) / float3(width, height, depth);
    
    // サンプラーは必ず Wrap を使用
    float3 currentVelocity = gVelocityRead.SampleLevel(gLinearWrapSampler, uvw, 0).xyz;

    // バックトレース
    float3 fluidSize = gFluidSettings.gridMax - gFluidSettings.gridMin;
    float3 deltaUVW = (currentVelocity * gFrameData.deltaTime / fluidSize);
    float3 backtraceUVW = uvw - deltaUVW;

    // 後ろに移動した位置の速度をサンプリング
    float3 velSemiLag = gVelocityRead.SampleLevel(gLinearWrapSampler, backtraceUVW, 0).xyz;

    // そこから逆に前へトレースし直す
    float3 forwardUVW = backtraceUVW + (velSemiLag * gFrameData.deltaTime / fluidSize);

    // 元の座標とのズレを計算し、サンプリング座標を補正
    float3 errorCorrection = uvw - forwardUVW;
    float3 bfeccUVW = backtraceUVW + errorCorrection * 0.5f; // エラーを半分補正

    // 補正されたUVWを使ってサンプリング
    float3 advectedVelocity = gVelocityRead.SampleLevel(gLinearWrapSampler, bfeccUVW, 0).xyz;
    float advectedDensity = gDensityRead.SampleLevel(gLinearWrapSampler, bfeccUVW, 0).r;

    // バックトレース先の位置の周囲のテクセルからMin/Maxを取得
    float3 texelSize = 1.0f / float3(width, height, depth);
    float minDensity = 9999.0f;
    float maxDensity = -9999.0f;

    // 簡易的なクロスサンプリング
    float3 offsets[4] =
    {
        float3(texelSize.x, 0, 0), float3(-texelSize.x, 0, 0),
    float3(0, texelSize.y, 0), float3(0, -texelSize.y, 0)
    };

    for (int i = 0; i < 4; ++i)
    {
        float sampleD = gDensityRead.SampleLevel(gLinearWrapSampler, backtraceUVW + offsets[i], 0).r;
        minDensity = min(minDensity, sampleD);
        maxDensity = max(maxDensity, sampleD);
    }

    // BFECCの結果が周囲の現実的な値を超えていたら、安全な1次移流にフォールバック、またはクランプ
    advectedDensity = clamp(advectedDensity, minDensity, maxDensity);
    
    // UVW座標自体の移流 と Toroidal Lerp
    float3 advectedUVW = gUVWRead.SampleLevel(gLinearWrapSampler, backtraceUVW, 0).xyz;

    // lerpではなく、トーラス境界を跨いだ最短経路で緩和
    float3 diff = advectedUVW - uvw;
    
    // 差分を -0.5 ～ 0.5 の範囲にラップし、最短経路のベクトルにする
    diff = diff - floor(diff + 0.5f);
    
    // 緩和係数を適用して、元のuvwに足し戻す
    float relaxationFactor = gFluidSettings.uvwRelaxation * gFrameData.deltaTime;
    float3 relaxedUVW = uvw + diff * (1.0f - relaxationFactor);
    
    // 最後に再び 0.0 ～ 1.0 の範囲に安全にラップ
    advectedUVW = relaxedUVW - floor(relaxedUVW);
    
    // 速度の大きさを取得
    float velLength = length(advectedVelocity);

    // 速度が速いほど減衰しにくく、遅いほど一気に消散させるハック
    float dynamicDissipation = lerp(gFluidSettings.velocityDissipation * 0.95f, gFluidSettings.velocityDissipation, saturate(velLength * 0.2f));

    advectedVelocity *= dynamicDissipation;

    // 書き込み
    gVelocityWrite[DTid.xyz] = float4(advectedVelocity, 0.0f);
    gDensityWrite[DTid.xyz] = advectedDensity;
    gUVWWrite[DTid.xyz] = float4(advectedUVW, 0.0f);
}