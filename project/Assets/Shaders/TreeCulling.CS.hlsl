#include "ShaderConstants.hlsli"

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
        return;
    
    TreeInstanceData tree = gInputTreeData[instanceIndex];
    
    // トランスフォーム行列の平行移動成分から直接ワールド座標を取得
    float3 rootPos = tree.worldMatrix[3].xyz;
    
    float treeHeight = gTreeCullingData.approxTreeHeight;
    float treeRadius = gTreeCullingData.approxTreeRadius;

    bool isVisible = true;

    float distToCam = distance(rootPos, gFrameData.cameraWorldPosition);
    if (distToCam > gTreeCullingData.maxDrawDistance)
    {
        isVisible = false;
    }

    if (isVisible)
    {
        // 樹木の形状を内包するためのバウンディングスフィア近似
        // 重心をY軸方向へオフセットした球判定に落とし込んで計算を軽量化
        float3 sphereCenter = rootPos + float3(0.0f, treeHeight * 0.5f, 0.0f);
        float boundsRadius = max(treeHeight * 0.5f, treeRadius) * 1.2f;
        
        for (uint i = 0; i < kFrustumPlaneCount; ++i)
        {
            if (dot(gTreeCullingData.frustumPlanes[i].xyz, sphereCenter) + gTreeCullingData.frustumPlanes[i].w < -boundsRadius)
            {
                isVisible = false;
                break;
            }
        }
    }

    // ---------------------------------------------------------
    // Smooth LOD Transition 用のフェード値算出
    // ---------------------------------------------------------
    // 遠景でモデルが急に消えるのを防ぐためのディザリング用アルファを計算
    // 頂点ごとの計算で済むよう、コンピュートシェーダーで事前計算しインスタンスデータに乗せておく
    if (isVisible)
    {
        float fadeStart = gTreeCullingData.maxDrawDistance * 0.8f;
        float fadeRange = gTreeCullingData.maxDrawDistance - fadeStart;
        tree.lodFade = saturate(1.0f - ((distToCam - fadeStart) / fadeRange));
    }

    // ---------------------------------------------------------
    // VRAMアクセス競合の回避 (Wave Intrinsics)
    // ---------------------------------------------------------
    // Wave 内で可視インスタンス数を集計し、代表の1スレッドのみがアトミック加算
    uint waveCount = WaveActiveCountBits(isVisible);
    uint waveOffset = 0;

    if (WaveIsFirstLane() && waveCount > 0)
    {
        gIndirectDrawArgs.InterlockedAdd(kIndirectInstanceCountOffset, waveCount, waveOffset);
    }

    waveOffset = WaveReadLaneFirst(waveOffset);

    if (isVisible)
    {
        // Wave内での自身の書き込みオフセットをプレフィックスサムで決定し、バッファを詰めて出力
        uint appendIndex = waveOffset + WavePrefixCountBits(isVisible);
        gOutputTreeData[appendIndex] = tree;
    }
}