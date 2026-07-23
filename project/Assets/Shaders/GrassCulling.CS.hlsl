#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<GrassCullingData> gGrassCullingData : register(b1);

// すべての草の基本データ（ワールド全体、またはカメラ周辺チャンク）
StructuredBuffer<GrassInstanceData> gInputGrassData : register(t0);

// 生き残った草を格納するバッファ
RWStructuredBuffer<GrassInstanceData> gOutputGrassData : register(u0);
// 生き残った草の数
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint instanceIndex = DTid.x;
    
    // バッファオーバーフロー防止
    if (instanceIndex >= gGrassCullingData.totalInstanceCount)
        return;
    
    GrassInstanceData grass = gInputGrassData[instanceIndex];
    float3 pos = grass.posAndHeight.xyz;

    // 1. 距離カリング (Distance Culling)
    float distToCam = distance(pos, gFrameData.cameraWorldPosition);
    if (distToCam > gGrassCullingData.maxDrawDistance)
        return;

    // 2. フラストゥムカリング (Frustum Culling)
    // 草のバウンディングスフィア（高さに応じた球体）で判定
    float boundsRadius = grass.posAndHeight.w * 1.2f;
    for (int i = 0; i < 6; ++i)
    {
        if (dot(gGrassCullingData.frustumPlanes[i].xyz, pos) + gGrassCullingData.frustumPlanes[i].w < -boundsRadius)
            return; // 視界外
    }

    // 3. 確率的間引き (Stochastic Thinning)
    // 遠くに行くほど一定の確率で草を非表示にする
    float thinFactor = saturate((distToCam - gGrassCullingData.thinStartDistance) / (gGrassCullingData.maxDrawDistance - gGrassCullingData.thinStartDistance));
    
    float randomVal = Hash12(pos.xz);
    if (thinFactor > 0.0f && randomVal < (thinFactor * gGrassCullingData.maxThinningRate))
    {
        return; // 間引き対象
    }

    // --- カリングを通過した草のみをストリーミングバッファへ書き込み ---
    uint appendIndex;
    // IndirectDrawArgs の ByteOffset 4 (InstanceCount) をアトミック加算
    gIndirectDrawArgs.InterlockedAdd(4, 1, appendIndex);

    gOutputGrassData[appendIndex] = grass;
}