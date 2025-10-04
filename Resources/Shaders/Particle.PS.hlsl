// 入力
struct VertexOut
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float textureIndex : TEXCOORD1;
};

// テクスチャ
Texture2DArray diffuseMapArray : register(t1);// 最大16枚まで
SamplerState sampler0 : register(s0);

// メイン
float4 main(VertexOut vin) : SV_TARGET
{
    // テクスチャを使う場合（透過や模様）
    float4 texColor = diffuseMapArray.Sample(sampler0, float3(vin.uv, int(vin.textureIndex + 0.5)));
    float4 finalColor = texColor * vin.color;

    // アルファが小さければ描画しない
    if (finalColor.a < 0.01)
        discard;

    return finalColor;

    // 単色
    //return vin.color;
}