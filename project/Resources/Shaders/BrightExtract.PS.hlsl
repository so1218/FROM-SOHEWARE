#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);
ConstantBuffer<BrightExtractSettings> gBrightExtractSettings : register(b0);

float4 main(VSOutput input) : SV_TARGET
{
    float4 color = tex.Sample(samLinear, input.uv);

    // 【変更点1】輝度(Luma)ではなく、RGBの最大値(MaxRGB)を使う
    // ネオンのような「高彩度な色」は、輝度計算すると暗くなりがちです（特に青）。
    // MaxRGBを使うことで、純粋な青(0,0,1)もしっかり「明るさ1.0」として扱えます。
    float brightness = max(color.r, max(color.g, color.b));

    // 【変更点2】Smoothstepを使った直感的な抽出
    // brightness が threshold 以下なら 0.0
    // brightness が threshold + softKnee 以上なら 1.0
    // その間は滑らかに補間 (これがSoftKneeの効果)
    float softKneeRange = max(gBrightExtractSettings.softKnee, 0.001f); // 0除算防止
    float glowFactor = smoothstep(gBrightExtractSettings.threshold, gBrightExtractSettings.threshold + softKneeRange, brightness);

    // 【変更点3】抽出
    // 元の色に glowFactor を掛けるだけ。シンプルです。
    float3 result = color.rgb * glowFactor;

    return float4(result * gBrightExtractSettings.intensity, 1.0);
}