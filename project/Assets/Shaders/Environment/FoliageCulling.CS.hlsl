#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FoliageCullingData> gCullingData : register(b1);

StructuredBuffer<FoliageInstanceData> gInputFoliage : register(t0);
// 生成した数。FoliageGeneration.CS の Append のカウンター
ByteAddressBuffer gGeneratedCount : register(t1);

RWStructuredBuffer<FoliageInstanceData> gOutputFoliage : register(u0);
// 描画引数。InstanceCount に見えた数を足していく
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

static const uint kThreadsPerRow = 1024 * 64;
// D3D12_DRAW_INDEXED_ARGUMENTS の InstanceCount の位置 (バイト)
static const uint kInstanceCountOffset = 4;
static const uint kFrustumPlaneCount = 6;
static const float kMinValidScale = 0.001f;

bool IsFoliageVisible(float3 rootPosition, float scale)
{
    if (scale <= kMinValidScale)
        return false;

    float distanceToCamera = distance(rootPosition.xz, gFrameData.cameraWorldPosition.xz);
    if (distanceToCamera > gCullingData.maxDrawDistance)
        return false;

    // 原点は根元にあるので、植物の中ほどを中心にした球で判定する
    float3 boundsCenter = rootPosition + float3(0.0f, gCullingData.modelCenterYOffset * scale, 0.0f);
    float boundsRadius = gCullingData.modelRadius * scale;
    for (uint i = 0; i < kFrustumPlaneCount; ++i)
    {
        if (dot(gFrameData.frustumPlanes[i].xyz, boundsCenter) + gFrameData.frustumPlanes[i].w < -boundsRadius)
            return false;
    }

    // 遠くほど間引く
    float thinRange = max(1.0f, gCullingData.maxDrawDistance - gCullingData.thinStartDistance);
    float thinRatio = saturate((distanceToCamera - gCullingData.thinStartDistance) / thinRange);
    return Hash12(rootPosition.xz) >= thinRatio * gCullingData.maxThinningRate;
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    // CPU は生成された数を知らないので、多めに起動して実際の数で打ち切る
    uint instanceIndex = dispatchThreadId.y * kThreadsPerRow + dispatchThreadId.x;
    if (instanceIndex >= gGeneratedCount.Load(0))
        return;

    FoliageInstanceData foliage = gInputFoliage[instanceIndex];
    bool isVisible = IsFoliageVisible(foliage.posAndScale.xyz, foliage.posAndScale.w);

    // 1つずつ足すと atomic が混み合うので、Wave ごとにまとめて書き込む場所を取る
    uint visibleCount = WaveActiveCountBits(isVisible);
    uint writeStart = 0;
    if (WaveIsFirstLane() && visibleCount > 0)
    {
        gIndirectDrawArgs.InterlockedAdd(kInstanceCountOffset, visibleCount, writeStart);
    }
    writeStart = WaveReadLaneFirst(writeStart);

    if (isVisible)
    {
        gOutputFoliage[writeStart + WavePrefixCountBits(isVisible)] = foliage;
    }
}
