#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelInjectCurrent : register(t0);
RWTexture3D<float4> gVoxelInjectFiltered : register(u0);
ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// 輝度を計算するヘルパー関数
// sRGB の輝度係数を使用し、人間の視覚特性に合わせた重み付けを行う
float CalculateLuminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

// 隣接ボクセルへのオフセットはグローバル定数化してレジスタ消費を抑える
static const int3 kNeighborOffsets[6] =
{
    int3(-1, 0, 0), int3(1, 0, 0), // 左右
    int3(0, -1, 0), int3(0, 1, 0), // 上下
    int3(0, 0, -1), int3(0, 0, 1) // 前後
};

// バイラテラルフィルターのLOD調整用定数
// 手前のボクセルはエッジを保持してクッキリさせる（感度高）
static const float kEdgeSensitivityNear = 2.0f;
// 奥のボクセルはアーティファクトを消すため強制的にボカす（感度低）
static const float kEdgeSensitivityFar = 0.02f;
// フェードを開始/終了するZ深度の割合
static const float kFadeStartRatio = 0.1f;
static const float kFadeEndRatio = 0.7f;

[numthreads(8, 8, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelInjectFiltered.GetDimensions(width, height, depth);
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    float4 center = gVoxelInjectCurrent.Load(int4(DTid, 0));
    float4 sum = center;
    float totalWeight = 1.0f;
    
    // Z深度に応じてエッジ保存の感度を下げる
    float zLinear = float(DTid.z) / float(depth - 1);
    float lodFade = smoothstep(kFadeStartRatio, kFadeEndRatio, zLinear);
    float bilateralSensitivity = lerp(kEdgeSensitivityNear, kEdgeSensitivityFar, lodFade);
    
    float centerLuma = CalculateLuminance(center.rgb);
    
    for (int i = 0; i < 6; ++i)
    {
        int3 neighborCoord = int3(DTid) + kNeighborOffsets[i];
        
        // 境界外アクセスを防ぐクランプ
        neighborCoord.x = clamp(neighborCoord.x, 0, int(width) - 1);
        neighborCoord.y = clamp(neighborCoord.y, 0, int(height) - 1);
        neighborCoord.z = clamp(neighborCoord.z, 0, int(depth) - 1);
        
        float4 neighbor = gVoxelInjectCurrent.Load(int4(neighborCoord, 0));
        
        // 色の差分と密度の差分でウェイトを計算
        float neighborLuma = CalculateLuminance(neighbor.rgb);
        float colorDiff = abs(centerLuma - neighborLuma) + abs(center.a - neighbor.a);
        
        float weight = exp(-colorDiff * bilateralSensitivity);
        
        sum += neighbor * weight;
        totalWeight += weight;
    }
    
    gVoxelInjectFiltered[DTid] = sum / totalWeight;
}