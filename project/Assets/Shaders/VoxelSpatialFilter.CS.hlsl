#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelInjectCurrent : register(t0);
RWTexture3D<float4> gVoxelInjectFiltered : register(u0);
ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// 輝度を計算するヘルパー関数
float CalculateLuminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

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
    
    // RDR2アプローチ：Z方向を含む効率的なサンプリング（星型3Dカーネル）
    // 自身の周囲（前後・左右・上下）をきっちりスムーズにぼかす
    int3 offsets[6] =
    {
        int3(-1, 0, 0), int3(1, 0, 0), // 左右
        int3(0, -1, 0), int3(0, 1, 0), // 上下
        int3(0, 0, -1), int3(0, 0, 1) // 前後（Z軸チカチカ撲滅用）
    };
    
    for (int i = 0; i < 6; ++i)
    {
        int3 neighborCoord = int3(DTid) + offsets[i];
        
        // 境界クランプ
        neighborCoord.x = clamp(neighborCoord.x, 0, int(width) - 1);
        neighborCoord.y = clamp(neighborCoord.y, 0, int(height) - 1);
        neighborCoord.z = clamp(neighborCoord.z, 0, int(depth) - 1);
        
        float4 neighbor = gVoxelInjectCurrent.Load(int4(neighborCoord, 0));
        
        // エッジ保存ウェイト（カラー差ベースのバイラテラル）
        // 差が激しい部分はボカさない（ライトのクッキリしたエッジやゴッドレイの筋を守る）
        float colorDiff = length(center.rgb - neighbor.rgb) + abs(center.a - neighbor.a);
        float weight = exp(-colorDiff * 2.0f); // 減衰感度は調整してください
        
        sum += neighbor * weight;
        totalWeight += weight;
    }
    
    gVoxelInjectFiltered[DTid] = sum / totalWeight;
}