#include "Trail.hlsli"

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    // C++側で既にワールド座標系でメッシュを作っている場合、
    // WVPは ViewProjection 行列のみが入っている想定
    output.position = mul(input.position, gTransformationMatrix.WVP);
    
    output.texcoord = input.texcoord;
    output.color = input.color; // C++で作ったフェード用カラーをパス
    return output;
}