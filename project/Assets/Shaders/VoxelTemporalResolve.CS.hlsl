#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelInjectFiltered : register(t0); // Spatial Filter後の今フレームのデータ
Texture3D<float4> gVoxelHistory : register(t1); // 前フレームのTemporal Resolve結果
RWTexture3D<float4> gVoxelTemporalOut : register(u0); // Temporal Resolveの出力（次の積算パスへ渡す）

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// TAA用 隣接ボクセルへのオフセット
static const int3 kNeighborOffsets[6] =
{
    int3(-1, 0, 0), int3(1, 0, 0),
    int3(0, -1, 0), int3(0, 1, 0),
    int3(0, 0, -1), int3(0, 0, 1)
};

// TAAブレンド用パラメータ (ゴーストとノイズの調整用)
static const float kBlendAlphaMin = 0.05f; // 静止時のブレンド率（履歴を95%信用しノイズを消去）
static const float kBlendAlphaMax = 0.4f; // 変化検出時のブレンド率（最新を40%採用し残像を防ぐ）
static const float kColorDiffThreshold = 2.0f; // この値以上の色差が発生したらブレンド率をMaxにする

// 深度からビュー空間のZを逆算する関数
float GetViewZFromSlice(float slice, float depthCount, float nearZ, float farZ)
{
    float zSlice = slice / depthCount;
    return nearZ * pow(farZ / nearZ, zSlice);
}

// ビュー空間のZからボクセルのスライスインデックスを逆算する関数
float GetSliceFromViewZ(float viewZ, float depthCount, float nearZ, float farZ)
{
    return (log2(viewZ / nearZ) / log2(farZ / nearZ)) * depthCount;
}

[numthreads(8, 8, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelInjectFiltered.GetDimensions(width, height, depth);
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    float4 current = gVoxelInjectFiltered.Load(int4(DTid, 0));

    // 周囲のボクセルから最小/最大の許容色を算出
    float4 boxMin = current;
    float4 boxMax = current;
    
    for (int i = 0; i < 6; ++i)
    {
        int3 neighborCoord = clamp(int3(DTid) + kNeighborOffsets[i], int3(0, 0, 0), int3(width - 1, height - 1, depth - 1));
        float4 neighbor = gVoxelInjectFiltered.Load(int4(neighborCoord, 0));
        boxMin = min(boxMin, neighbor);
        boxMax = max(boxMax, neighbor);
    }
    
    // カメラの移動を考慮したリプロジェクション
    float nearZ = max(gFrameData.nearClip, kMinNearClip);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);
    
    // 現在のボクセル中心の画面UVとビューZを計算
    float u = (float(DTid.x) + 0.5f) / float(width);
    float v = (float(DTid.y) + 0.5f) / float(height);
    float viewZ = GetViewZFromSlice(float(DTid.z) + 0.5f, float(depth), nearZ, farZ);
    
    // クリップ空間のXY
    float clipX = u * 2.0f - 1.0f;
    float clipY = (1.0f - v) * 2.0f - 1.0f;
    
    // 現在のワールド空間座標を復元
    float4 worldTarget = mul(float4(clipX, clipY, 1.0f, 1.0f), gFrameData.invViewProj);
    float3 rayDir = normalize(worldTarget.xyz / worldTarget.w - gFrameData.cameraWorldPosition);
    float3 worldPos = gFrameData.cameraWorldPosition + (rayDir * viewZ);
    
    // 復元したワールド座標を前フレームのクリップ空間に投影
    float4 prevClip = mul(float4(worldPos, 1.0f), gFrameData.prevViewProj);
    prevClip.xyz /= prevClip.w;
    
    // 前フレームの画面UVに変換
    float2 prevUV = prevClip.xy * float2(0.5f, -0.5f) + 0.5f;
    
    // 前フレームのカメラから見た距離を計算
    float3 prevCamToPos = worldPos - gFrameData.prevCameraWorldPosition;
    float prevViewZ = length(prevCamToPos);
    
    // 前フレームのボクセルテクスチャ上のインデックスに変換
    int3 historyCoord;
    historyCoord.x = int(prevUV.x * float(width));
    historyCoord.y = int(prevUV.y * float(height));
    historyCoord.z = int(GetSliceFromViewZ(prevViewZ, float(depth), nearZ, farZ));

    // 履歴フェッチ (画面外や限界外にはみ出た場合は現フレームを強制採用)
    float4 history = current;
    if (historyCoord.x >= 0 && historyCoord.x < int(width) &&
        historyCoord.y >= 0 && historyCoord.y < int(height) &&
        historyCoord.z >= 0 && historyCoord.z < int(depth))
    {
        history = gVoxelHistory.Load(int4(historyCoord, 0));
    }

    // カラークランピング（ゴースト除去の要）
    // 過去の色が現在の周囲の色から逸脱している場合、強制的に現在の範囲に収める
    history = clamp(history, boxMin, boxMax);

    // ブレンド率（TAAウェイト）の動的制御
    // 色の差分が大きい（動的オブジェクトの通過など）場合は履歴を捨てて残像を防ぐ
    float diff = length(current.rgb - history.rgb);
    float blendAlpha = lerp(kBlendAlphaMin, kBlendAlphaMax, saturate(diff / kColorDiffThreshold));

    float4 resolved = lerp(history, current, blendAlpha);

    gVoxelTemporalOut[DTid] = resolved;
}