#include "ShaderConstants.hlsli"

ConstantBuffer<GrassGenerationData> gGenerationData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b1);

// テクスチャとサンプラー
Texture2D<float> gHeightMap : register(t0); // 地形のハイトマップ
Texture2D<float> gDensityMap : register(t1); // 草の密度マスク（生える場所=白、道=黒）
SamplerState gLinearSampler : register(s0);

// 出力用バッファ
RWStructuredBuffer<GrassInstanceData> gOutputGrass : register(u0);

// ★追加: C++側の groupX (1024) * numthreads (64)
static const uint THREADS_PER_ROW = 1024 * 64;

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
    uint instanceIndex = DTid.y * THREADS_PER_ROW + DTid.x;
    
    if (instanceIndex >= gGenerationData.maxGrassPerChunk)
        return;
    // 1. カメラ（terrainCenter）の位置をグリッドのサイズでスナップして固定化する
    // これにより、カメラが少し動いても、一定距離進むまで基準位置がピタッと固定されます。
    float2 snappedCenter = floor(gGenerationData.terrainCenter / gGenerationData.gridSpacing) * gGenerationData.gridSpacing;

    // グリッド計算
    uint gridSizeX = (uint) ceil(gGenerationData.terrainWidth / gGenerationData.gridSpacing);
    if (gridSizeX == 0)
        gridSizeX = 1;

    uint gridX = instanceIndex % gridSizeX;
    uint gridZ = instanceIndex / gridSizeX;
    float2 localPos = float2(gridX * gGenerationData.gridSpacing, gridZ * gGenerationData.gridSpacing);
    
    float offsetX = -gGenerationData.terrainWidth * 0.5f;
    float offsetZ = -gGenerationData.terrainDepth * 0.5f;

    // 2. スナップされた中心座標を基準に、草の基本となるワールド座標を計算
    float baseWorldX = snappedCenter.x + offsetX + localPos.x;
    float baseWorldZ = snappedCenter.y + offsetZ + localPos.y;

    // 3. 乱数のシード（Jitter）は、完全に固定された baseWorld 座標を元に計算する（ここが一番重要）
    float2 jitter = Hash22(float2(baseWorldX, baseWorldZ)) * (gGenerationData.gridSpacing * 0.5f);
    
    // 4. 最終的な草のワールド座標
    float worldX = baseWorldX + jitter.x;
    float worldZ = baseWorldZ + jitter.y;
    
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
        
        // ★追加: step関数を使って、グレーを完全な白(1.0)か黒(0.0)に二値化する
        // 値が 0.5 より大きければ 1.0(草が生える)、小さければ 0.0(生えない) になる
        density = step(0.5f, density);

        if (randomVal > density)
        {
            isValid = false;
        }
    }

    // ★無効な場合は、必ずバッファを0で上書きして古いゴミデータを消す
    if (!isValid)
    {
        GrassInstanceData emptyGrass = (GrassInstanceData) 0;
        gOutputGrass[instanceIndex] = emptyGrass;
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

    gOutputGrass[instanceIndex] = grass;
}