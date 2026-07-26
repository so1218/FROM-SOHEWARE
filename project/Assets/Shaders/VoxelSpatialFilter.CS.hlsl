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
    
    // 遠方に行くほどエッジ保存の感度を下げる
    // 手前（zLinear=0）はクッキリ（2.0）、奥（zLinear=1）は強制全ボカシ（0.02）
    float zLinear = float(DTid.z) / float(depth - 1);
    float bilateralSensitivity = lerp(2.0f, 0.02f, smoothstep(0.1f, 0.7f, zLinear));
    
    int3 offsets[6] =
    {
        int3(-1, 0, 0), int3(1, 0, 0), // 左右
        int3(0, -1, 0), int3(0, 1, 0), // 上下
        int3(0, 0, -1), int3(0, 0, 1) // 前後
    };
    
    for (int i = 0; i < 6; ++i)
    {
        int3 neighborCoord = int3(DTid) + offsets[i];
        
        neighborCoord.x = clamp(neighborCoord.x, 0, int(width) - 1);
        neighborCoord.y = clamp(neighborCoord.y, 0, int(height) - 1);
        neighborCoord.z = clamp(neighborCoord.z, 0, int(depth) - 1);
        
        float4 neighbor = gVoxelInjectCurrent.Load(int4(neighborCoord, 0));
        
        float colorDiff = length(center.rgb - neighbor.rgb) + abs(center.a - neighbor.a);
        
        // 遠方は sensitivity が極小になるため、差が激しくても weight が 0 にならない
        float weight = exp(-colorDiff * bilateralSensitivity);
        
        sum += neighbor * weight;
        totalWeight += weight;
    }
    
    gVoxelInjectFiltered[DTid] = sum / totalWeight;
}