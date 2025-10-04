struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    float2 positions[3] =
    {
        float2(-1.0f, -3.0f),
        float2(-1.0f, 1.0f),
        float2(3.0f, 1.0f)
    };

    output.position = float4(positions[vertexID], 0.0f, 1.0f);
    // クリップ空間のxyを[0,1]にマッピングしてUVにする
    output.uv = float2(output.position.x, -output.position.y) * 0.5f + 0.5f;

    return output;
}