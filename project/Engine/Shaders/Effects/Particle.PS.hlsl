struct ParticleVSOutput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float textureIndex : TEXCOORD1;
};

Texture2D<float4> gTextures[] : register(t1);
SamplerState sampler0 : register(s0);

float4 main(ParticleVSOutput vin) : SV_TARGET
{
    uint texID = (uint) (vin.textureIndex + 0.5f);
    // テクスチャを使う場合
    float4 texColor = gTextures[NonUniformResourceIndex(texID)].Sample(sampler0, vin.uv);
    float4 finalColor = texColor * vin.color;

    // アルファが小さければ描画しない
    if (finalColor.a < 0.01)
        discard;

    return finalColor;
}