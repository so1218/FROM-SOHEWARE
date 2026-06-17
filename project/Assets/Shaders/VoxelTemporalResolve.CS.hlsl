#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelInjectFiltered : register(t0); // Spatial Filter後の今フレームのデータ
Texture3D<float4> gVoxelHistory : register(t1); // 前フレームのTemporal Resolve結果（歴史）
RWTexture3D<float4> gVoxelTemporalOut : register(u0); // Temporal Resolveの出力（これを次の積算パスへ渡す）

ConstantBuffer<FrameData> gFrameData : register(b0);

[numthreads(8, 8, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelInjectFiltered.GetDimensions(width, height, depth);
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    float4 current = gVoxelInjectFiltered.Load(int4(DTid, 0));

    // RDR2仕様：3D空間近傍（前後左右上下）から、より厳密なboxMin / boxMaxを作る
    float4 boxMin = current;
    float4 boxMax = current;

    int3 offsets[6] =
    {
        int3(-1, 0, 0), int3(1, 0, 0),
        int3(0, -1, 0), int3(0, 1, 0),
        int3(0, 0, -1), int3(0, 0, 1) // Z方向（前後）も絶対に含める
    };

    for (int i = 0; i < 6; ++i)
    {
        int3 neighborCoord = clamp(int3(DTid) + offsets[i], int3(0, 0, 0), int3(width - 1, height - 1, depth - 1));
        float4 neighbor = gVoxelInjectFiltered.Load(int4(neighborCoord, 0));
        boxMin = min(boxMin, neighbor);
        boxMax = max(boxMax, neighbor);
    }

    // 前フレームのリプロジェクション（同じ位置でもクランプが強力なら機能します）
    int3 historyCoord = DTid;
    float4 history = gVoxelHistory.Load(int4(historyCoord, 0));

    // 強力なカラークランピング（これで砂嵐ノイズの大部分が消滅します）
    history = clamp(history, boxMin, boxMax);

    // ブレンド率（TAAウェイト）の動的制御
    // 激しい変化がある場合は滑らかに現フレームの追従性を高める（if文によるカクつきを排除）
    float diff = length(current.rgb - history.rgb);
    float blendAlpha = lerp(0.05f, 0.4f, saturate(diff / 2.0f)); // 変化量に応じて0.05～0.4へ滑らかに遷移

    float4 resolved = lerp(history, current, blendAlpha);

    gVoxelTemporalOut[DTid] = resolved;
}