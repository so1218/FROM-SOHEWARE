#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<MaterialData> gMaterial : register(b5);
Texture2D<float4> gDissolveTexture : register(t4); 
SamplerState gSampler : register(s0);

struct ShadowVSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// ディザリング用の閾値マトリクス
float DitherThreshold4x4(float2 pixelPos)
{
    const float4x4 thresholdMatrix =
    {
        1.0 / 17.0, 9.0 / 17.0, 3.0 / 17.0, 11.0 / 17.0,
        13.0 / 17.0, 5.0 / 17.0, 15.0 / 17.0, 7.0 / 17.0,
        4.0 / 17.0, 12.0 / 17.0, 2.0 / 17.0, 10.0 / 17.0,
        16.0 / 17.0, 8.0 / 17.0, 14.0 / 17.0, 6.0 / 17.0
    };
    
    // ピクセル座標を4で割った余りを使ってマトリクスを参照
    int x = int(fmod(pixelPos.x, 4.0));
    int y = int(fmod(pixelPos.y, 4.0));
    return thresholdMatrix[x][y];
}

void main(ShadowVSOutput input)
{
    // ディゾルブ処理
    if (gMaterial.enableDissolve != 0)
    {
        float noise = gDissolveTexture.Sample(gSampler, input.texcoord).r;
        if (noise <= gMaterial.dissolveThreshold)
        {
            discard;
        }
    }

    // Alpha値によるディザリング
    float alpha = gMaterial.color.a;

    // もしアルファが1.0(完全不透明)未満なら、ディザリング
    if (alpha < 1.0f)
    {
        // 画面上のピクセル位置に基づいて閾値を取得 
        float threshold = DitherThreshold4x4(input.position.xy);

        // アルファ値が閾値より低ければ捨てる
        if (alpha < threshold)
        {
            discard;
        }
    }
}