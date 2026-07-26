#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GrassCullingData> gGrassCullingData : register(b1);

StructuredBuffer<GrassInstanceData> gInputGrassData : register(t0);
RWStructuredBuffer<GrassInstanceData> gOutputGrassData : register(u0);

// Indirect ArgsのInstanceCount(オフセット4バイト目)をインクリメントして描画数を動的決定
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

static const uint THREADS_PER_ROW = 1024 * 64;

// 疑似乱数 (Hash)
float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint instanceIndex = DTid.y * THREADS_PER_ROW + DTid.x;
    
    if (instanceIndex >= gGrassCullingData.totalInstanceCount)
        return;
    
    GrassInstanceData grass = gInputGrassData[instanceIndex];
    float3 pos = grass.posAndHeight.xyz;
    float height = grass.posAndHeight.w;

    // カリング判定 (距離 / 視錐台 / 確率的間引き)
    bool isVisible = true;

    // 生成フェーズで間引かれた無効インスタンスの早期リジェクト
    if (height <= 0.001f)
    {
        isVisible = false;
    }
    else
    {
        // 距離カリング
        float distToCamXZ = distance(pos.xz, gFrameData.cameraWorldPosition.xz);
        if (distToCamXZ > gGrassCullingData.maxDrawDistance)
        {
            isVisible = false;
        }

        // 視錐台カリング
        float boundsRadius = height * 1.2f;
        for (int i = 0; i < 6; ++i)
        {
            if (dot(gGrassCullingData.frustumPlanes[i].xyz, pos) + gGrassCullingData.frustumPlanes[i].w < -boundsRadius)
            {
                isVisible = false;
            }
        }

        // 確率的カリング: 遠景の密度を下げてLOD遷移を滑らかにする
        if (isVisible)
        {
            float fadeRange = max(1.0f, gGrassCullingData.maxDrawDistance - gGrassCullingData.thinStartDistance);
            float thinFactor = saturate((distToCamXZ - gGrassCullingData.thinStartDistance) / fadeRange);
            
            if (thinFactor > 0.0f && Hash12(pos.xz) < (thinFactor * gGrassCullingData.maxThinningRate))
            {
                isVisible = false;
            }
        }
    }

    // Wave IntrinsicsによるAppend最適化
    // 競合を減らすため、Wave内の有効スレッド数をまとめてアトミック加算
    uint waveCount = WaveActiveCountBits(isVisible);
    uint waveOffset = 0;

    // Waveの先頭スレッドが代表してグローバルバッファを更新
    if (WaveIsFirstLane() && waveCount > 0)
    {
        gIndirectDrawArgs.InterlockedAdd(4, waveCount, waveOffset);
    }

    // 取得したベースオフセットをWave内の全スレッドへブロードキャスト
    waveOffset = WaveReadLaneFirst(waveOffset);

    if (isVisible)
    {
        // プレフィックスサムを用いて各スレッドの出力先インデックスを決定
        uint appendIndex = waveOffset + WavePrefixCountBits(isVisible);
        gOutputGrassData[appendIndex] = grass;
    }
}