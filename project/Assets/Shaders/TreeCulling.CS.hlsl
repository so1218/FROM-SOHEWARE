#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<TreeCullingData> gTreeCullingData : register(b1);

StructuredBuffer<TreeInstanceData> gInputTreeData : register(t0);
RWStructuredBuffer<TreeInstanceData> gOutputTreeData : register(u0);
RWByteAddressBuffer gIndirectDrawArgs : register(u1);

static const uint kThreadsPerRow = 65536;
static const uint kIndirectInstanceCountOffset = 4; // InstanceCountのオフセット
static const uint kFrustumPlaneCount = 6;

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint instanceIndex = DTid.y * kThreadsPerRow + DTid.x;
    
    if (instanceIndex >= gTreeCullingData.totalInstanceCount)
        return;
    
    TreeInstanceData tree = gInputTreeData[instanceIndex];
    
    // 行列からワールド座標(根元)を抽出
    float3 rootPos = tree.worldMatrix[3].xyz;
    
    // 木のおおよその高さと半径（定数バッファから取得、または行列のスケールから計算）
    float treeHeight = gTreeCullingData.approxTreeHeight;
    float treeRadius = gTreeCullingData.approxTreeRadius;

    bool isVisible = true;

    // 距離カリング
    float distToCam = distance(rootPos, gFrameData.cameraWorldPosition);
    if (distToCam > gTreeCullingData.maxDrawDistance)
    {
        isVisible = false;
    }

    // 2. 視錐台(フラスタム)カリング
    if (isVisible)
    {
        // 木の根元ではなく、木の中央を球の中心にする
        float3 sphereCenter = rootPos + float3(0.0f, treeHeight * 0.5f, 0.0f);
        
        // 球の半径（高さの半分と半径の大きい方 + 余裕をもたせる）
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

    // 3. LOD フェード値の動的計算 (オプション)
    // 遠くの木ほど lodFade を下げていき、VSやPSでディザリング・アルファ抜きに使う
    if (isVisible)
    {
        float fadeStart = gTreeCullingData.maxDrawDistance * 0.8f;
        float fadeRange = gTreeCullingData.maxDrawDistance - fadeStart;
        
        // 1.0(完全表示) ～ 0.0(消える) の値を instanceData に書き込む
        tree.lodFade = 1.0f - saturate((distToCam - fadeStart) / fadeRange);
    }

    // ---------------------------------------------------------
    // Wave Intrinsics によるアトミック競合の回避 (Grassと同じ)
    // ---------------------------------------------------------
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
        // 更新した lodFade を含めて出力バッファへ書き込み
        gOutputTreeData[appendIndex] = tree;
    }
}