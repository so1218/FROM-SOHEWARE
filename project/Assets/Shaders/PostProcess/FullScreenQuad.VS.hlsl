#include "Common/FullScreenQuad.hlsli"

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // 全画面三角形の座標とUV座標を頂点IDに応じて設定
    float2 positions[3] =
    {
        float2(-1.0f, -3.0f),
        float2(-1.0f, 1.0f),
        float2(3.0f, 1.0f)
    };

    output.position = float4(positions[vertexID], 0.0f, 1.0f);
    output.uv = float2((positions[vertexID].x + 1.0f) * 0.5f, 1.0f - ((positions[vertexID].y + 1.0f) * 0.5f));

    return output;
}