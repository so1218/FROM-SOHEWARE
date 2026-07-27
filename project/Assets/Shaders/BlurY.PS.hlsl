#include "ShaderConstants.hlsli"
#include "FullScreenQuad.hlsli"

Texture2D gTexture : register(t0);
SamplerState samLinear : register(s0);

ConstantBuffer<BlurSettings> gBlurSettings : register(b0);

static const float offset[3] = { 0.0, 1.3846153846, 3.2307692308 };
static const float weight[3] = { 0.2270270270, 0.3162162162, 0.0702702703 };

float4 main(VSOutput input) : SV_TARGET
{
    // 中心ピクセル
    float4 color = gTexture.Sample(samLinear, input.uv) * weight[0];
    
    for (int i = 1; i < 3; ++i)
    {
        float2 offsetUV = float2(0.0, offset[i] * gBlurSettings.texelSize.y * gBlurSettings.blurStrength);

        color += gTexture.Sample(samLinear, input.uv + offsetUV) * weight[i];
        color += gTexture.Sample(samLinear, input.uv - offsetUV) * weight[i];
    }

    return color;
}