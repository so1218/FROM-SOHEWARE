#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GrassCullingData> gGrassCullingData : register(b1);

StructuredBuffer<GrassInstanceData> gInputGrassData : register(t0);
RWStructuredBuffer<GrassInstanceData> gOutputGrassData : register(u0);

// 描画引数。InstanceCount に見えた本数を足していく
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

// Dispatch の X は 65535 までなので、Y に折り返している
static const uint kThreadsPerRow = 65536;
// D3D12_DRAW_ARGUMENTS の InstanceCount の位置 (バイト)
static const uint kInstanceCountOffset = 4;
static const uint kFrustumPlaneCount = 6;
static const float kMinValidHeight = 0.001f;
// 風で傾いた分も入るように、高さより少し大きくする
static const float kBoundsRadiusScale = 1.2f;

bool IsGrassVisible(float3 position, float height)
{
    // 生成のときに生やさなかった場所は高さが 0
    if (height <= kMinValidHeight)
        return false;

    float distanceToCameraXZ = distance(position.xz, gFrameData.cameraWorldPosition.xz);
    if (distanceToCameraXZ > gGrassCullingData.maxDrawDistance)
        return false;

    // 草の中ほどを中心にした球で判定する
    float3 boundsCenter = position + float3(0.0f, height * 0.5f, 0.0f);
    float boundsRadius = height * kBoundsRadiusScale;
    for (uint i = 0; i < kFrustumPlaneCount; ++i)
    {
        if (dot(gFrameData.frustumPlanes[i].xyz, boundsCenter) + gFrameData.frustumPlanes[i].w < -boundsRadius)
            return false;
    }

    // 遠くほど間引く。描画側では間引いた分だけ幅を広げている
    float thinRange = max(1.0f, gGrassCullingData.maxDrawDistance - gGrassCullingData.thinStartDistance);
    float thinRatio = saturate((distanceToCameraXZ - gGrassCullingData.thinStartDistance) / thinRange);
    return Hash12(position.xz) >= thinRatio * gGrassCullingData.maxThinningRate;
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint instanceIndex = dispatchThreadId.y * kThreadsPerRow + dispatchThreadId.x;
    if (instanceIndex >= gGrassCullingData.totalInstanceCount)
        return;

    GrassInstanceData grass = gInputGrassData[instanceIndex];
    bool isVisible = IsGrassVisible(grass.posAndHeight.xyz, grass.posAndHeight.w);

    // 1本ずつ足すと atomic が混み合うので、Wave ごとにまとめて書き込む場所を取る
    uint visibleCount = WaveActiveCountBits(isVisible);
    uint writeStart = 0;
    if (WaveIsFirstLane() && visibleCount > 0)
    {
        gIndirectDrawArgs.InterlockedAdd(kInstanceCountOffset, visibleCount, writeStart);
    }
    writeStart = WaveReadLaneFirst(writeStart);

    if (isVisible)
    {
        gOutputGrassData[writeStart + WavePrefixCountBits(isVisible)] = grass;
    }
}
