#include "ShaderConstants.hlsli"

ConstantBuffer<GrassGenerationData> gGenerationData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b1);

// テクスチャとサンプラー
Texture2D<float> gHeightMap : register(t0); // 地形のハイトマップ
Texture2D<float> gDensityMap : register(t1); // 草の密度マスク（生える場所=白、道=黒）
SamplerState gLinearSampler : register(s0);

// 出力用バッファ
RWStructuredBuffer<GrassInstanceData> gOutputGrass : register(u0);

// ワールド座標(x, z)から地形全体のUVを計算する関数
float2 CalculateTerrainUV(float x, float z)
{
    // 地形の左上を原点としたローカル座標に変換し、全体のサイズで割って0.0〜1.0にする
    float u = (x - gGenerationData.terrainCenter.x) / gGenerationData.terrainWidth + 0.5f;
    float v = (z - gGenerationData.terrainCenter.y) / gGenerationData.terrainDepth + 0.5f;
    return float2(u, v);
}

// 2入力1出力（例: 座標から高さの乱数を作る）
float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

// 2入力2出力（例: 座標から x,z のJitterズレを作る）
float2 Hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xx + p3.yz) * p3.zy);
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= gGenerationData.maxGrassPerChunk)
        return;

    // グリッド計算
    uint gridSizeX = (uint) ceil(gGenerationData.terrainWidth / gGenerationData.gridSpacing);
    if (gridSizeX == 0)
        gridSizeX = 1;

    uint gridX = DTid.x % gridSizeX;
    uint gridZ = DTid.x / gridSizeX;
    float2 localPos = float2(gridX * gGenerationData.gridSpacing, gridZ * gGenerationData.gridSpacing);
    
    // 地形の「中心」を基準にするオフセット
    float offsetX = -gGenerationData.terrainWidth * 0.5f;
    float offsetZ = -gGenerationData.terrainDepth * 0.5f;

    float2 jitter = Hash22(localPos) * (gGenerationData.gridSpacing * 0.5f);
    
    float worldX = gGenerationData.terrainCenter.x + offsetX + localPos.x + jitter.x;
    float worldZ = gGenerationData.terrainCenter.y + offsetZ + localPos.y + jitter.y;

    float2 globalUV = CalculateTerrainUV(worldX, worldZ);

    // ==========================================
    // ★修正: 途中で return せず、isValid フラグで管理する
    // ==========================================
    bool isValid = true;

    // 1. 地形の範囲外チェック
    if (globalUV.x < 0.0f || globalUV.x > 1.0f || globalUV.y < 0.0f || globalUV.y > 1.0f)
    {
        isValid = false;
    }

    // 2. 密度マップによる間引きチェック
    float randomVal = Hash12(float2(worldX, worldZ));
    if (isValid)
    {
        float density = gDensityMap.SampleLevel(gLinearSampler, globalUV, 0).r;
        if (randomVal > density)
        {
            isValid = false;
        }
    }

    // ★無効な場合は、必ずバッファを0で上書きして古いゴミデータを消す
    if (!isValid)
    {
        GrassInstanceData emptyGrass = (GrassInstanceData) 0;
        gOutputGrass[DTid.x] = emptyGrass;
        return;
    }

    // ==========================================
    // 有効な草のデータ構築
    // ==========================================
    float heightRatio = gHeightMap.SampleLevel(gLinearSampler, globalUV, 0).r - 0.5f;
    float worldY = heightRatio * gTerrainSettings.maxHeight;

    float randomHeight = lerp(gGenerationData.minHeight, gGenerationData.maxHeight, Hash12(float2(worldZ, worldX)));
    float randomWidth = lerp(gGenerationData.minWidth, gGenerationData.maxWidth, Hash12(float2(worldX, worldZ)));
    float randomRotY = Hash12(float2(worldX, worldZ)) * 3.14159265f * 2.0f;
    uint packedColor = 0xFFFFFFFF;

    GrassInstanceData grass;
    grass.posAndHeight = float4(worldX, worldY, worldZ, randomHeight);
    grass.rotWidthColor = float4(randomRotY, randomWidth, packedColor, 0.0f);

    gOutputGrass[DTid.x] = grass;
}