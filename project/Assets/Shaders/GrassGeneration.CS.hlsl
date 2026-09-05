#include "Common/ShaderConstants.hlsli"

ConstantBuffer<GrassGenerationData> gGenerationData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b1);

Texture2D<float> gHeightMap : register(t0);
Texture2D<float> gDensityMap : register(t1);
SamplerState gLinearSampler : register(s0);

RWStructuredBuffer<GrassInstanceData> gOutputGrass : register(u0);

// C++側の Dispatch(1024, 1, 1) と numthreads(64, 1, 1) の総数
static const uint kThreadsPerRow = 1024 * 64;

// ワールド座標から地形全体の0-1 UVへのマッピング
float2 CalculateTerrainUV(float x, float z)
{
    float u = (x - gGenerationData.terrainCenter.x) / gGenerationData.terrainWidth + 0.5f;
    float v = (z - gGenerationData.terrainCenter.y) / gGenerationData.terrainDepth + 0.5f;
    return float2(u, v);
}

// 疑似乱数 (Hash)
float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

float2 Hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xx + p3.yz) * p3.zy);
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint instanceIndex = DTid.y * kThreadsPerRow + DTid.x;
    
    if (instanceIndex >= gGenerationData.maxGrassPerChunk)
        return;

    // グリッド座標計算
    // カメラ移動に伴う草のチラツキを防ぐため、グリッド単位でスナップした座標を基準に
    float2 snappedCenter = floor(gGenerationData.terrainCenter / gGenerationData.gridSpacing) * gGenerationData.gridSpacing;

    uint gridSizeX = max((uint) ceil(gGenerationData.terrainWidth / gGenerationData.gridSpacing), 1);
    uint gridX = instanceIndex % gridSizeX;
    uint gridZ = instanceIndex / gridSizeX;
    float2 localPos = float2(gridX * gGenerationData.gridSpacing, gridZ * gGenerationData.gridSpacing);
    
    float offsetX = -gGenerationData.terrainWidth * 0.5f;
    float offsetZ = -gGenerationData.terrainDepth * 0.5f;

    float baseWorldX = snappedCenter.x + offsetX + localPos.x;
    float baseWorldZ = snappedCenter.y + offsetZ + localPos.y;

    // スナップされた安定した座標をシードに、配置のランダムな位置ズレを適用
    float2 jitter = Hash22(float2(baseWorldX, baseWorldZ)) * (gGenerationData.gridSpacing * 0.5f);
    
    float worldX = baseWorldX + jitter.x;
    float worldZ = baseWorldZ + jitter.y;
    
    float2 globalUV = CalculateTerrainUV(worldX, worldZ);
    
    // カリング判定
    bool isValid = true;

    // 領域外カリング
    if (any(globalUV < 0.0f) || any(globalUV > 1.0f))
    {
        isValid = false;
    }

    // 密度マップによる確率的カリング
    if (isValid)
    {
        float density = gDensityMap.SampleLevel(gLinearSampler, globalUV, 0).r;
        density = step(0.5f, density); // Maskの二値化による曖昧な境界ノイズの排除

        float randomVal = Hash12(float2(worldX, worldZ));
        if (randomVal > density)
        {
            isValid = false;
        }
    }

    // カリングされたインスタンスは、前フレームのゴミを残さないよう明示的にゼロクリア
    if (!isValid)
    {
        gOutputGrass[instanceIndex] = (GrassInstanceData) 0;
        return;
    }

    // インスタンスデータの生成
    float heightRatio = gHeightMap.SampleLevel(gLinearSampler, globalUV, 0).r - 0.5f;
    float worldY = heightRatio * gTerrainSettings.maxHeight;

    // 各種ランダムパラメータの生成
    float randomHeight = lerp(gGenerationData.minHeight, gGenerationData.maxHeight, Hash12(float2(worldZ, worldX)));
    float randomWidth = lerp(gGenerationData.minWidth, gGenerationData.maxWidth, Hash12(float2(worldX, worldZ)));
    float randomRotY = Hash12(float2(worldX, worldZ)) * PI * 2.0f;
    
    uint packedColor = 0xFFFFFFFF; // 初期値

    GrassInstanceData grass;
    grass.posAndHeight = float4(worldX, worldY, worldZ, randomHeight);
    grass.rotWidthColor = float4(randomRotY, randomWidth, packedColor, 0.0f);

    gOutputGrass[instanceIndex] = grass;
}