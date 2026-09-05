#include "Common/ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FoliageCullingData> gCullingData : register(b1);

StructuredBuffer<FoliageInstanceData> gInputFoliage : register(t0);

// GenerationCSのAppendカウンタ(u0)から直接値を読み取る
ByteAddressBuffer gInstanceCounter : register(t1);

RWStructuredBuffer<FoliageInstanceData> gOutputFoliage : register(u0);
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

static const uint kThreadsPerRow = 65536;
static const uint kIndirectInstanceCountOffset = 4;
static const uint kFrustumPlaneCount = 6;

float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint instanceIndex = DTid.y * kThreadsPerRow + DTid.x;
    
    // CPU側でのディスパッチ数の無駄をカバーするため、GPU側の実生成数で早期リターン
    uint totalGeneratedCount = gInstanceCounter.Load(0);
    if (instanceIndex >= totalGeneratedCount)
        return;
    
    FoliageInstanceData foliage = gInputFoliage[instanceIndex];
    float3 pos = foliage.posAndScale.xyz;
    float scale = foliage.posAndScale.w;

    bool isVisible = (scale > 0.001f);

    if (isVisible)
    {
        // XZ平面のみで距離計算
        float distToCamXZ = distance(pos.xz, gFrameData.cameraWorldPosition.xz);
        
        if (distToCamXZ > gCullingData.maxDrawDistance)
        {
            isVisible = false;
        }

        if (isVisible)
        {
            float boundsRadius = gCullingData.modelRadius * scale;
            
            // 植物モデルの原点は根元(Y=0)にあるため、カリング用スフィアの中心を葉の重心付近へ持ち上げる
            float3 boundsCenter = pos + float3(0.0f, gCullingData.modelCenterYOffset * scale, 0.0f);
            
            for (uint i = 0; i < kFrustumPlaneCount; ++i)
            {
                if (dot(gFrameData.frustumPlanes[i].xyz, boundsCenter) + gFrameData.frustumPlanes[i].w < -boundsRadius)
                {
                    isVisible = false;
                    break;
                }
            }
        }

        // 遠景のオーバードローおよび、サブピクセル級ポリゴンによるラスタライザの処理落ちを防ぐ
        if (isVisible)
        {
            float fadeRange = max(1.0f, gCullingData.maxDrawDistance - gCullingData.thinStartDistance);
            float thinFactor = saturate((distToCamXZ - gCullingData.thinStartDistance) / fadeRange);
            
            if (thinFactor > 0.0f && Hash12(pos.xz) < (thinFactor * gCullingData.maxThinningRate))
            {
                isVisible = false;
            }
        }
    }

    // ----------------------------------------------------------------------
    // Wave Intrinsics によるアトミック操作の最適化 (Atomic Contention の回避)
    // ----------------------------------------------------------------------
    // 全スレッドが個別に InterlockedAdd を呼ぶと、VRAMへのアクセス競合で大渋滞を起こす。
    // そのため、Wave(SIMDグループ)内で生き残った数を集計し、最初の1スレッドだけがメモリアクセスを行う。
    uint waveCount = WaveActiveCountBits(isVisible);
    uint waveOffset = 0;

    if (WaveIsFirstLane() && waveCount > 0)
    {
        gIndirectDrawArgs.InterlockedAdd(kIndirectInstanceCountOffset, waveCount, waveOffset);
    }
    
    // リーダースレッドが取得した書き込み開始位置を、Wave内の全スレッドへ共有
    waveOffset = WaveReadLaneFirst(waveOffset);

    if (isVisible)
    {
        // プレフィックスサムを用いて、このWave内での自身の相対的な書き込みインデックスを決定
        uint appendIndex = waveOffset + WavePrefixCountBits(isVisible);
        gOutputFoliage[appendIndex] = foliage;
    }
}