#include "Trail.hlsli"

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    // 位置をビュー射影行列で変換
    output.position = mul(input.position, gTransformationMatrix.WVP);

    // UVにスクロールを適用
    float2 scroll = gTrailMaterial.scrollSpeed * gFrameData.gTime;
    output.texcoord = input.texcoord + scroll;

    output.color = input.color;

    return output;
}