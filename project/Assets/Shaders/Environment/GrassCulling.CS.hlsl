#include "Common/ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GrassCullingData> gGrassCullingData : register(b1);

StructuredBuffer<GrassInstanceData> gInputGrassData : register(t0);
RWStructuredBuffer<GrassInstanceData> gOutputGrassData : register(u0);

// Indirect ArgsのInstanceCount(オフセット4バイト目)をインクリメントして描画数を動的決定
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

static const uint kThreadsPerRow = 65536; // Dispatch(X)上限(65535)回避用2D展開幅
static const uint kIndirectInstanceCountOffset = 4; // IndirectArgsバッファ内のInstanceCountオフセット(Byte)
static const uint kFrustumPlaneCount = 6;
static const float kMinValidHeight = 0.001f;
static const float kBoundsRadiusScale = 1.2f; // 判定バウンディングスフィアの余裕持たせ

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
    // 2Dスレッドインデックスから1DのインスタンスIDを復元
    uint instanceIndex = DTid.y * kThreadsPerRow + DTid.x;
    
    if (instanceIndex >= gGrassCullingData.totalInstanceCount)
        return;
    
    GrassInstanceData grass = gInputGrassData[instanceIndex];
    float3 pos = grass.posAndHeight.xyz;
    float height = grass.posAndHeight.w;

    // ---------------------------------------------------------
    // カリング判定 (早期リジェクト)
    // ---------------------------------------------------------
    bool isVisible = true;

    // 非アクティブ（高さゼロ）草の早期除外
    if (height <= kMinValidHeight)
    {
        isVisible = false;
    }
    else
    {
        // 距離カリング (XZ平面)
        float distToCamXZ = distance(pos.xz, gFrameData.cameraWorldPosition.xz);
        if (distToCamXZ > gGrassCullingData.maxDrawDistance)
        {
            isVisible = false;
        }

        // 視錐台カリング (球判定)
        if (isVisible)
        {
            float boundsRadius = height * kBoundsRadiusScale;
            
            for (uint i = 0; i < kFrustumPlaneCount; ++i)
            {
                if (dot(gFrameData.frustumPlanes[i].xyz, pos) + gFrameData.frustumPlanes[i].w < -boundsRadius)
                {
                    isVisible = false;
                    break; // 1平面でも外側なら確定で離脱
                }
            }
        }

        // 確率的ディザカリング (遠景LODフェードアウト & 密度調整)
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

    // ---------------------------------------------------------
    // Wave Intrinsics による Append 競合緩和
    // ---------------------------------------------------------
    // レーン単位ではなく、Wave 全体でまとめてアトミック加算してバスボトルネックを回避
    uint waveCount = WaveActiveCountBits(isVisible);
    uint waveOffset = 0;

    if (WaveIsFirstLane() && waveCount > 0)
    {
        gIndirectDrawArgs.InterlockedAdd(kIndirectInstanceCountOffset, waveCount, waveOffset);
    }

    // 代表スレッドが取得した書き込み開始アドレスをWave内へブロードキャスト
    waveOffset = WaveReadLaneFirst(waveOffset);

    if (isVisible)
    {
        // Wave内プレフィックスサムで自身の出力インデックスを決定
        uint appendIndex = waveOffset + WavePrefixCountBits(isVisible);
        gOutputGrassData[appendIndex] = grass;
    }
}