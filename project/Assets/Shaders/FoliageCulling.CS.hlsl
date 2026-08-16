#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FoliageCullingData> gCullingData : register(b1);

// ★ 読み込み元（Generation CSで生成されたバッファ）
StructuredBuffer<FoliageInstanceData> gInputFoliage : register(t0);

// ★ 追加：GenerationCSで記録された「実際の」インスタンス数（Appendカウンタ）
ByteAddressBuffer gInstanceCounter : register(t1);

// ★ 書き込み先（カリングを生き残った描画用バッファ）
RWStructuredBuffer<FoliageInstanceData> gOutputFoliage : register(u0);
// ★ 間接描画の引数バッファ
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

static const uint kThreadsPerRow = 65536;
static const uint kIndirectInstanceCountOffset = 4; // DrawInstancedIndirect の InstanceCount の位置
static const uint kFrustumPlaneCount = 6;

// 擬似乱数 (Hash12)
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
    
// ★ 修正：GPU上にあるカウンタバッファから実際の生成数を取得
    uint totalGeneratedCount = gInstanceCounter.Load(0);
    
    if (instanceIndex >= totalGeneratedCount)
        return; // 生成された数以上のスレッドは即座に終了
    
    FoliageInstanceData foliage = gInputFoliage[instanceIndex];
    
    float3 pos = foliage.posAndScale.xyz;
    float scale = foliage.posAndScale.w;

    bool isVisible = true;

    // 1. スケールによる安全なリジェクト
    if (scale <= 0.001f)
    {
        isVisible = false;
    }
    else
    {
        // 2. 距離カリング
        float distToCamXZ = distance(pos.xz, gFrameData.cameraWorldPosition.xz);
        
        if (distToCamXZ > gCullingData.maxDrawDistance)
        {
            isVisible = false;
        }

        // 3. 視錐台カリング (Foliage専用の最適化)
        if (isVisible)
        {
            // ★ FoliageはPebbleのような「非等方スケール(Aniso)」を持たないため、計算を簡略化して軽量化
            float boundsRadius = gCullingData.modelRadius * scale;
            
            // ★ 植物のピボット（根元）から、モデルの中心（葉っぱのあたり）へオフセットする
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

        // 4. 確率的ディザカリング (遠景の密度を間引いて負荷を下げる)
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

    // ==========================================
    // ★ Wave Intrinsics による超高速 Append
    // ==========================================
    uint waveCount = WaveActiveCountBits(isVisible);
    uint waveOffset = 0;

    if (WaveIsFirstLane() && waveCount > 0)
    {
        gIndirectDrawArgs.InterlockedAdd(kIndirectInstanceCountOffset, waveCount, waveOffset);
    }
    waveOffset = WaveReadLaneFirst(waveOffset);

    if (isVisible)
    {
        // このウェーブ内での自分の書き込み位置をプレフィックスサムで取得
        uint appendIndex = waveOffset + WavePrefixCountBits(isVisible);
        gOutputFoliage[appendIndex] = foliage;
    }
}