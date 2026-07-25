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
    float height = grass.posAndHeight.w;

 // --- カリング判定 ---
    bool isVisible = true;

// ★追加: 高さが0（生成時に間引かれた無効な草）なら即座に除外
    if (height <= 0.001f)
    {
        isVisible = false;
    }
    else
    {
// 1. 距離カリング
        float distToCamXZ = distance(pos.xz, gFrameData.cameraWorldPosition.xz);
        
        if (distToCamXZ > gGrassCullingData.maxDrawDistance)
        {
            isVisible = false;
        }

// 2. フラストゥムカリング (高さが有効な場合のみ計算)
        float boundsRadius = height * 1.2f;
        for (int i = 0; i < 6; ++i)
        {
            if (dot(gGrassCullingData.frustumPlanes[i].xyz, pos) + gGrassCullingData.frustumPlanes[i].w < -boundsRadius)
                isVisible = false;
        }

// 3. 確率的間引き
        if (isVisible)
        {
            // ゼロ除算を防止しつつ、XZ距離でフェード割合を計算
            float fadeRange = max(1.0f, gGrassCullingData.maxDrawDistance - gGrassCullingData.thinStartDistance);
            float thinFactor = saturate((distToCamXZ - gGrassCullingData.thinStartDistance) / fadeRange);
            
            if (thinFactor > 0.0f && Hash12(pos.xz) < (thinFactor * gGrassCullingData.maxThinningRate))
            {
                isVisible = false;
            }
        }
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