#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FoliageCullingData> gCullingData : register(b1);

StructuredBuffer<FoliageInstanceData> gInputFoliage : register(t0);
ByteAddressBuffer gInstanceCounter : register(t1);   // 生成した数 (FoliageGeneration.CS の Append カウンター)

RWStructuredBuffer<FoliageInstanceData> gOutputFoliage : register(u0);
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

static const uint kThreadsPerRow = 65536;
static const uint kIndirectInstanceCountOffset = 4;
static const uint kFrustumPlaneCount = 6;

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // CPU は生成した数を知らないので、多めに起動して実際の数で打ち切る
    uint instanceIndex = DTid.y * kThreadsPerRow + DTid.x;
    if (instanceIndex >= gInstanceCounter.Load(0))
    {
        return;
    }

    FoliageInstanceData foliage = gInputFoliage[instanceIndex];
    float3 rootPosition = foliage.posAndScale.xyz;
    float scale = foliage.posAndScale.w;

    float distanceToCamera = distance(rootPosition.xz, gFrameData.cameraWorldPosition.xz);
    bool isVisible = scale > 0.001f && distanceToCamera <= gCullingData.maxDrawDistance;

    // 原点は根元にあるので、球の中心を植物の中ほどまで上げて視錐台と判定する
    if (isVisible)
    {
        float3 sphereCenter = rootPosition + float3(0.0f, gCullingData.modelCenterYOffset * scale, 0.0f);
        float sphereRadius = gCullingData.modelRadius * scale;

        for (uint planeIndex = 0; planeIndex < kFrustumPlaneCount; ++planeIndex)
        {
            float4 plane = gFrameData.frustumPlanes[planeIndex];
            if (dot(plane.xyz, sphereCenter) + plane.w < -sphereRadius)
            {
                isVisible = false;
                break;
            }
        }
    }

    // 遠くほど間引いて、描く数を減らす
    if (isVisible)
    {
        float thinRange = max(1.0f, gCullingData.maxDrawDistance - gCullingData.thinStartDistance);
        float thinRate = saturate((distanceToCamera - gCullingData.thinStartDistance) / thinRange) * gCullingData.maxThinningRate;
        isVisible = Hash12(rootPosition.xz) >= thinRate;
    }

    // 見えるものだけを出力に詰める。
    // アトミック加算は Wave (同時に動くスレッドのまとまり) ごとに 1 回だけ行う
    uint visibleCountInWave = WaveActiveCountBits(isVisible);
    uint waveOffset = 0;
    if (WaveIsFirstLane() && visibleCountInWave > 0)
    {
        gIndirectDrawArgs.InterlockedAdd(kIndirectInstanceCountOffset, visibleCountInWave, waveOffset);
    }
    waveOffset = WaveReadLaneFirst(waveOffset);

    if (isVisible)
    {
        gOutputFoliage[waveOffset + WavePrefixCountBits(isVisible)] = foliage;
    }
}
