#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<PebbleCullingData> gCullingData : register(b1);
StructuredBuffer<PebbleInstanceData> gInputPebble : register(t0);
RWStructuredBuffer<PebbleInstanceData> gOutputPebble : register(u0);
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

static const uint kThreadsPerRow = 65536;
static const uint kIndirectInstanceCountOffset = 4; 
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
    
    if (instanceIndex >= gCullingData.totalInstanceCount)
        return;
    
    PebbleInstanceData pebble = gInputPebble[instanceIndex];
    
    float3 pos = pebble.posAndScale.xyz;
    float scale = pebble.posAndScale.w;

    bool isVisible = true;

    // ★ 非アクティブの除外 (Grassの早期リジェクト相当)
    if (scale <= 0.001f)
    {
        isVisible = false;
    }
    else
    {
        // 1. 距離カリング
        float distToCamXZ = distance(pos.xz, gFrameData.cameraWorldPosition.xz);
        
        if (distToCamXZ > gCullingData.maxDrawDistance)
        {
            isVisible = false;
        }

        // 2. 視錐台カリング (Grassと同一の球判定)
        if (isVisible)
        {
            // ★修正: 画面端で不自然に消えるのを防ぐため、異方性スケールの最大値を取り、安全マージンを掛ける
            float maxAniso = max(max(pebble.anisoAndEmbed.x, pebble.anisoAndEmbed.y), pebble.anisoAndEmbed.z);
            float boundsRadius = gCullingData.modelRadius * scale * maxAniso * 1.5f;
            
            // ピボット位置(pos)からY軸中心へオフセット
            float3 boundsCenter = pos + float3(0.0f, gCullingData.modelCenterYOffset * scale * pebble.anisoAndEmbed.y, 0.0f);
            
            for (uint i = 0; i < kFrustumPlaneCount; ++i)
            {
                if (dot(gFrameData.frustumPlanes[i].xyz, boundsCenter) + gFrameData.frustumPlanes[i].w < -boundsRadius)
                {
                    isVisible = false;
                    break;
                }
            }
        }

        // 3. 確率的ディザカリング (Grassと完全同一)
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
    // Wave Intrinsics による Append
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
        uint appendIndex = waveOffset + WavePrefixCountBits(isVisible);
        gOutputPebble[appendIndex] = pebble;
    }
}