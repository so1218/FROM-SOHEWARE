#include "ShaderConstants.hlsli"

Texture2D<float> gInputDepth : register(t0); // 読み込み元 (親Mip)
RWTexture2D<float> gOutputDepth : register(u0); // 書き込み先 (子Mip)

// Point + Clamp サンプラー (C++側で Static Sampler等で設定しておく)
SamplerState gPointClampSampler : register(s0);

cbuffer HiZSettings : register(b0)
{
    // uint2 のサイズの代わりに、UV計算用の逆数 (1.0 / InputSize) を渡す
    float2 gInvInputSize;
    uint2 gOutputSize;
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // 出力解像度外のスレッドを破棄
    if (any(DTid.xy >= gOutputSize))
        return;

    // 2x2ピクセルブロックの「中心座標」のUVを計算
    // 例: DTid=(0,0) の場合、親の (1.0, 1.0) ピクセルの位置を指す
    float2 uv = (DTid.xy * 2.0f + 1.0f) * gInvInputSize;

    // GatherRed命令: 指定したUVの周囲2x2ピクセルの赤チャンネルを1命令で取得する
    // (戻り値 float4 の x, y, z, w にそれぞれ左上、右上、左下、右下の値が入る)
    // 境界のクランプは gPointClampSampler がハードウェアレベルで自動処理してくれる
    float4 depths = gInputDepth.GatherRed(gPointClampSampler, uv);

    // 2x2内で最も手前の深度を残す
    float minDepth = min(min(depths.x, depths.y), min(depths.z, depths.w));

    gOutputDepth[DTid.xy] = minDepth;
}