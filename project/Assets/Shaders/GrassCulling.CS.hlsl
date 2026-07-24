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

   // --- カリング判定 ---
    bool isVisible = true;

    // 1. 距離カリング
    float distToCam = distance(pos, gFrameData.cameraWorldPosition);
    if (distToCam > gGrassCullingData.maxDrawDistance)
        isVisible = false;

    // 2. フラストゥムカリング
    float boundsRadius = grass.posAndHeight.w * 1.2f;
    for (int i = 0; i < 6; ++i)
    {
        if (dot(gGrassCullingData.frustumPlanes[i].xyz, pos) + gGrassCullingData.frustumPlanes[i].w < -boundsRadius)
            isVisible = false;
    }

    // 3. 確率的間引き
    float thinFactor = saturate((distToCam - gGrassCullingData.thinStartDistance) / (gGrassCullingData.maxDrawDistance - gGrassCullingData.thinStartDistance));
    float randomVal = Hash12(pos.xz);
    if (thinFactor > 0.0f && randomVal < (thinFactor * gGrassCullingData.maxThinningRate))
    {
        isVisible = false;
    }

    // ==========================================
    // ★ Wave Intrinsics による超高速アトミック加算
    // ==========================================
    
    // Wave（64スレッド）の中で、isVisible が true になっているスレッド数をカウント
    uint waveCount = WaveActiveCountBits(isVisible);
    uint waveOffset = 0;

    // Wave内の「先頭の有効なスレッド」だけが、まとめてアトミック加算を行う
    if (WaveIsFirstLane() && waveCount > 0)
    {
        gIndirectDrawArgs.InterlockedAdd(4, waveCount, waveOffset);
    }

    // 取得したベースとなるインデックス（waveOffset）を、Wave内の全スレッドに共有
    waveOffset = WaveReadLaneFirst(waveOffset);

    // 生き残った草だけをバッファに書き込む
    if (isVisible)
    {
        // WavePrefixCountBits は「自分より若いIDのスレッドで true になっている数」を返す
        uint appendIndex = waveOffset + WavePrefixCountBits(isVisible);
        gOutputGrassData[appendIndex] = grass;
    }
}