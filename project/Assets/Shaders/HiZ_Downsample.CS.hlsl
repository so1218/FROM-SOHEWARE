#include "ShaderConstants.hlsli"

// 1つ上のMipレベルの深度バッファ
Texture2D<float> gInputDepth : register(t0);

// 現在書き込んでいるMipレベルの深度バッファ
RWTexture2D<float> gOutputDepth : register(u0);

cbuffer HiZSettings : register(b0)
{
    uint2 gInputSize; // 入力元（親Mip）の解像度
    uint2 gOutputSize; // 出力先（子Mip）の解像度
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // 出力先の画面外スレッドは処理しない
    if (DTid.x >= gOutputSize.x || DTid.y >= gOutputSize.y)
        return;

    // 入力元（親Mip）の左上ピクセル座標を計算 (2倍のスケール)
    uint2 baseCoord = DTid.xy * 2;

    // 2x2のピクセルをロードする
    // ※親が奇数解像度（例: 1025x1025）だった場合のエラーを防ぐため、最大値でクランプ(min)します
    uint2 coord00 = min(baseCoord + uint2(0, 0), gInputSize - 1);
    uint2 coord10 = min(baseCoord + uint2(1, 0), gInputSize - 1);
    uint2 coord01 = min(baseCoord + uint2(0, 1), gInputSize - 1);
    uint2 coord11 = min(baseCoord + uint2(1, 1), gInputSize - 1);

    // MipLevelを指定せずに直接ピクセル座標からLoadする
    float d00 = gInputDepth.Load(int3(coord00, 0));
    float d10 = gInputDepth.Load(int3(coord10, 0));
    float d01 = gInputDepth.Load(int3(coord01, 0));
    float d11 = gInputDepth.Load(int3(coord11, 0));

    // --- 【最重要】深度の抽出 ---
    // DX12の標準的な深度バッファ（Near: 0.0, Far: 1.0）の場合、
    // 「値が小さい＝カメラに近い」となるため、4つのうち最小値(min)を取得します。
    float minDepth = min(min(d00, d10), min(d01, d11));

    gOutputDepth[DTid.xy] = minDepth;
}