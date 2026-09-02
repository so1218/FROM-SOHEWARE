#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelInjectFiltered : register(t0); // Spatial Filter後の今フレームのデータ
Texture3D<float4> gVoxelHistory : register(t1); // 前フレームのTemporal Resolve結果
RWTexture3D<float4> gVoxelTemporalOut : register(u0); // Temporal Resolveの出力（次の積算パスへ渡す）

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

SamplerState gLinearClampSampler : register(s0);

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
static const float kColorDiffThreshold = 2.0f; // この値以上の色差が発生したらブレンド率をMaxに

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

    // 時間蓄積時のゴーストを抑制するため、3x3x3近傍の平均と分散からカラー境界(AABB)を算出。
    // ※現状27回のLoadはL1キャッシュに依存しているため、将来的にGroupSharedMemory(LDS)への移行余地あり。
    float4 m1 = 0.0f;
    float4 m2 = 0.0f;

    for (int z = -1; z <= 1; ++z)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                int3 neighborCoord = clamp(int3(DTid) + int3(x, y, z), int3(0, 0, 0), int3(width - 1, height - 1, depth - 1));
                float4 neighbor = gVoxelInjectFiltered.Load(int4(neighborCoord, 0));
                m1 += neighbor;
                m2 += neighbor * neighbor;
            }
        }
    }

    float4 mean = m1 / 27.0f;
    float4 stddev = sqrt(max(m2 / 27.0f - mean * mean, 0.0f));

    // 注入ノイズ(IGN等)の高周波成分を誤ってクリップしないよう、gamma を広めに設定
    float gamma = 2.2f;
    float4 boxMin = mean - gamma * stddev;
    float4 boxMax = mean + gamma * stddev;

    // 現在のボクセルからワールド座標を逆算し、前フレームのカメラ行列を用いてリプロジェクション
    float nearZ = max(gFrameData.nearClip, kMinNearClip);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);

    float u = (float(DTid.x) + 0.5f) / float(width);
    float v = (float(DTid.y) + 0.5f) / float(height);
    float viewZ = GetViewZFromSlice(float(DTid.z) + 0.5f, float(depth), nearZ, farZ);

    float clipX = u * 2.0f - 1.0f;
    float clipY = (1.0f - v) * 2.0f - 1.0f;

    float4 worldTarget = mul(float4(clipX, clipY, 1.0f, 1.0f), gFrameData.invViewProj);
    float3 rayDir = normalize(worldTarget.xyz / worldTarget.w - gFrameData.cameraWorldPosition);
    float3 worldPos = gFrameData.cameraWorldPosition + (rayDir * viewZ);

    // 前フレームのUVWを計算し履歴をフェッチ
    float4 prevClip = mul(float4(worldPos, 1.0f), gFrameData.prevViewProj);
    prevClip.xyz /= prevClip.w;

    float2 prevUV = prevClip.xy * float2(0.5f, -0.5f) + 0.5f;
    float3 prevCamToPos = worldPos - gFrameData.prevCameraWorldPosition;
    float prevViewZ = length(prevCamToPos);

    float prevSlice = GetSliceFromViewZ(prevViewZ, float(depth), nearZ, farZ);
    float3 prevUVW = float3(prevUV.x, prevUV.y, prevSlice / float(depth));

    float4 history = current;

    // 履歴が有効範囲内であればサンプリングし、Variance Clippingで境界内に収める (色滲みの防止)
    if (all(prevUVW >= 0.0f) && all(prevUVW <= 1.0f))
    {
        history = gVoxelHistory.SampleLevel(gLinearClampSampler, prevUVW, 0);
    }
    history = clamp(history, boxMin, boxMax);

    // 輝度の変化量(動的オブジェクトの移動や急な照明変化)に応じてブレンド率を適応的に上げ、残像を逃がす
    float diff = length(current.rgb - history.rgb);
    float blendAlpha = lerp(kBlendAlphaMin, kBlendAlphaMax, saturate(diff / kColorDiffThreshold));

    gVoxelTemporalOut[DTid] = lerp(history, current, blendAlpha);
}