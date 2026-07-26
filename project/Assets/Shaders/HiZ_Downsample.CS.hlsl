#include "ShaderConstants.hlsli"

Texture2D<float> gInputDepth : register(t0); // 読み込み元 (親Mip)
RWTexture2D<float> gOutputDepth : register(u0); // 書き込み先 (子Mip)

cbuffer HiZSettings : register(b0)
{
    uint2 gInputSize;
    uint2 gOutputSize;
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // 出力解像度外のスレッドを破棄
    if (any(DTid.xy >= gOutputSize))
        return;

    // 親Mipにおける2x2ピクセルブロックの基準座標
    uint2 baseCoord = DTid.xy * 2;

    // 奇数解像度からのダウンサンプリング時に生じる境界外アクセスをクランプで防止
    uint2 coord00 = min(baseCoord + uint2(0, 0), gInputSize - 1);
    uint2 coord10 = min(baseCoord + uint2(1, 0), gInputSize - 1);
    uint2 coord01 = min(baseCoord + uint2(0, 1), gInputSize - 1);
    uint2 coord11 = min(baseCoord + uint2(1, 1), gInputSize - 1);

    float d00 = gInputDepth.Load(int3(coord00, 0));
    float d10 = gInputDepth.Load(int3(coord10, 0));
    float d01 = gInputDepth.Load(int3(coord01, 0));
    float d11 = gInputDepth.Load(int3(coord11, 0));

    // オクルージョンカリング用の深度構築
    // オブジェクトが隠蔽されるか保守的に判定するため、2x2内で最も手前の深度を残す (Near:0.0, Far:1.0)
    float minDepth = min(min(d00, d10), min(d01, d11));

    gOutputDepth[DTid.xy] = minDepth;
}