struct VSOutput
{
    float4 position : SV_POSITION; // 変換後のクリップ空間座標
    float depth : TEXCOORD0; // 深度値（クリップ空間の深度）
};
