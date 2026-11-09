#include "Common/ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<TreeCullingData> gTreeCullingData : register(b1);

StructuredBuffer<TreeInstanceData> gInputTreeData : register(t0);
RWStructuredBuffer<TreeInstanceData> gOutputTreeData : register(u0);
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

static const uint kThreadsPerRow = 65536;
static const uint kIndirectInstanceCountOffset = 4;
static const uint kFrustumPlaneCount = 6;

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint instanceIndex = DTid.y * kThreadsPerRow + DTid.x;
    if (instanceIndex >= gTreeCullingData.totalInstanceCount)
    {
        return;
    }

    TreeInstanceData tree = gInputTreeData[instanceIndex];
    float3 rootPosition = tree.worldMatrix[3].xyz;

    float distanceToCamera = distance(rootPosition, gFrameData.cameraWorldPosition);
    bool isVisible = distanceToCamera <= gTreeCullingData.maxDrawDistance;

    // 木を、根元から高さの半分の位置を中心とした球で近似して視錐台と判定する
    if (isVisible)
    {
        float treeHeight = gTreeCullingData.approxTreeHeight;
        float3 sphereCenter = rootPosition + float3(0.0f, treeHeight * 0.5f, 0.0f);
        float sphereRadius = max(treeHeight * 0.5f, gTreeCullingData.approxTreeRadius) * 1.2f;

        for (uint planeIndex = 0; planeIndex < kFrustumPlaneCount; ++planeIndex)
        {
            float4 plane = gTreeCullingData.frustumPlanes[planeIndex];
            if (dot(plane.xyz, sphereCenter) + plane.w < -sphereRadius)
            {
                isVisible = false;
                break;
            }
        }
    }

    // 描画距離の最後の 2 割で、ディザで少しずつ消す
    if (isVisible)
    {
        float fadeStart = gTreeCullingData.maxDrawDistance * 0.8f;
        float fadeRange = gTreeCullingData.maxDrawDistance - fadeStart;
        tree.lodFade = saturate(1.0f - (distanceToCamera - fadeStart) / fadeRange);
    }

    // 見える木だけを出力に詰める。
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
        gOutputTreeData[waveOffset + WavePrefixCountBits(isVisible)] = tree;
    }
}
