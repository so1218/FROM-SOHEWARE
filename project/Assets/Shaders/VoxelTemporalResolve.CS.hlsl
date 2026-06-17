#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelInjectFiltered : register(t0); // Spatial Filter後の今フレームのデータ
Texture3D<float4> gVoxelHistory : register(t1); // 前フレームのTemporal Resolve結果（歴史）
RWTexture3D<float4> gVoxelTemporalOut : register(u0); // Temporal Resolveの出力（これを次の積算パスへ渡す）

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);
// 深度（スライス）からビュー空間のZ（奥方向の距離）を逆算する関数（Injectionと同じロジック）
float GetViewZFromSlice(float slice, float depthCount, float nearZ, float farZ)
{
    float zSlice = slice / depthCount;
    return nearZ * pow(farZ / nearZ, zSlice);
}

// ビュー空間のZから、ボクセルのスライスインデックス（Z座標）を逆算する関数
float GetSliceFromViewZ(float viewZ, float depthCount, float nearZ, float farZ)
{
    return (log(viewZ / nearZ) / log(farZ / nearZ)) * depthCount;
}

[numthreads(8, 8, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelInjectFiltered.GetDimensions(width, height, depth);
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    float4 current = gVoxelInjectFiltered.Load(int4(DTid, 0));

    // 1. 周囲のクランプボックス算出 (これは今のままでOK)
    float4 boxMin = current;
    float4 boxMax = current;
    int3 offsets[6] =
    {
        int3(-1, 0, 0), int3(1, 0, 0),
        int3(0, -1, 0), int3(0, 1, 0),
        int3(0, 0, -1), int3(0, 0, 1)
    };
    for (int i = 0; i < 6; ++i)
    {
        int3 neighborCoord = clamp(int3(DTid) + offsets[i], int3(0, 0, 0), int3(width - 1, height - 1, depth - 1));
        float4 neighbor = gVoxelInjectFiltered.Load(int4(neighborCoord, 0));
        boxMin = min(boxMin, neighbor);
        boxMax = max(boxMax, neighbor);
    }

    // ====================================================================
    // 2. RDR2方式：カメラの移動を考慮したリプロジェクション（履歴座標の逆算）
    // ====================================================================
    float nearZ = max(gFrameData.nearClip, 0.1f);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);
    
    // 現在のボクセル中心の画面UVと、そこから得られるビューZを計算
    float u = (float(DTid.x) + 0.5f) / float(width);
    float v = (float(DTid.y) + 0.5f) / float(height);
    float viewZ = GetViewZFromSlice(float(DTid.z) + 0.5f, float(depth), nearZ, farZ);
    
    // クリップ空間のXY
    float clipX = u * 2.0f - 1.0f;
    float clipY = (1.0f - v) * 2.0f - 1.0f;
    
    // 現在のボクセルの「ワールド空間座標」を復元
    float4 worldTarget = mul(float4(clipX, clipY, 1.0f, 1.0f), gFrameData.invViewProj);
    float3 rayDir = normalize(worldTarget.xyz / worldTarget.w - gFrameData.cameraWorldPosition);
    float3 worldPos = gFrameData.cameraWorldPosition + (rayDir * viewZ);
    
    // 復元したワールド座標を「前フレームのカメラ画面空間」に投影
    float4 prevClip = mul(float4(worldPos, 1.0f), gFrameData.prevViewProj);
    prevClip.xyz /= prevClip.w;
    
    // 前フレームの画面UVに変換
    float2 prevUV = prevClip.xy * float2(0.5f, -0.5f) + 0.5f;
    
    // 前フレームのカメラから見たビュー空間Zを計算
    // （前フレームのカメラ位置からの距離、または前フレームのビュー行列のZ軸への射影）
    // 簡易的には距離で近似、または前フレームのView行列があるならそれを使用
    float3 prevCamToPos = worldPos - gFrameData.prevCameraWorldPosition; // ※C++から前フレームカメラ位置も貰うと正確
    float prevViewZ = length(prevCamToPos); // 簡易的な距離ベース。あるいは前View行列でのZ
    
    // 前フレームのボクセルテクスチャ上のインデックス(XYZ)に変換
    int3 historyCoord;
    historyCoord.x = int(prevUV.x * float(width));
    historyCoord.y = int(prevUV.y * float(height));
    historyCoord.z = int(GetSliceFromViewZ(prevViewZ, float(depth), nearZ, farZ));

    // 画面外や描画限界外にはみ出た場合は、クランプするか現フレームを強制採用する
    float4 history = current;
    if (historyCoord.x >= 0 && historyCoord.x < int(width) &&
        historyCoord.y >= 0 && historyCoord.y < int(height) &&
        historyCoord.z >= 0 && historyCoord.z < int(depth))
    {
        // 過去の正しいワールド位置からデータをロード
        history = gVoxelHistory.Load(int4(historyCoord, 0));
    }
    // ====================================================================

    // 強力なカラークランピング（位置が同期したため、動いても不必要に削られなくなります）
    history = clamp(history, boxMin, boxMax);

    // ブレンド率（TAAウェイト）の動的制御
    float diff = length(current.rgb - history.rgb);
    float blendAlpha = lerp(0.05f, 0.4f, saturate(diff / 2.0f));

    float4 resolved = lerp(history, current, blendAlpha);

    gVoxelTemporalOut[DTid] = resolved;
}