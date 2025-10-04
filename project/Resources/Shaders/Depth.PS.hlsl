Texture2D<float> depthTexture : register(t0);
SamplerState samplerLinear : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer CameraSettingsPS : register(b1)
{
    float nearClipPS;
    float farClipPS;
    float2 paddingPS;
};

float4 main(VSOutput input) : SV_TARGET
{
    // 深度テクスチャから深度値をサンプル
    float depth = depthTexture.Sample(samplerLinear, input.uv);

    // 深度値は0〜1に正規化されているのでそのまま使えます
    // 必要に応じて線形化する場合はここで計算

    return float4(depth, depth, depth, 1.0f);
}