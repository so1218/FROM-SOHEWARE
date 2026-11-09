#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

Texture3D<float4> gVoxelInjectCurrent : register(t0);
RWTexture3D<float4> gVoxelInjectFiltered : register(u0);
ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

static const int3 kNeighborOffsets[6] =
{
    int3(-1, 0, 0), int3(1, 0, 0),
    int3(0, -1, 0), int3(0, 1, 0),
    int3(0, 0, -1), int3(0, 0, 1)
};

// 奥ほど鈍くして、遠くのちらつきを強くならす
static const float kEdgeSensitivityNear = 2.0f;
static const float kEdgeSensitivityFar = 0.02f;
static const float kFadeStartRatio = 0.1f;
static const float kFadeEndRatio = 0.7f;

// 隣の 6 つのうち、明るさが近いものほど強く混ぜる
[numthreads(8, 8, 4)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint froxelWidth, froxelHeight, froxelDepth;
    gVoxelInjectFiltered.GetDimensions(froxelWidth, froxelHeight, froxelDepth);
    if (any(dispatchThreadId >= uint3(froxelWidth, froxelHeight, froxelDepth)))
        return;

    int3 maxCoord = int3(froxelWidth, froxelHeight, froxelDepth) - 1;

    float4 center = gVoxelInjectCurrent.Load(int4(dispatchThreadId, 0));
    float centerLuminance = CalculateLuminance(center.rgb);

    float depthRatio = float(dispatchThreadId.z) / float(max(1u, froxelDepth - 1));
    float sensitivity = lerp(kEdgeSensitivityNear, kEdgeSensitivityFar, smoothstep(kFadeStartRatio, kFadeEndRatio, depthRatio));

    float4 sum = center;
    float totalWeight = 1.0f;
    for (int i = 0; i < 6; ++i)
    {
        int3 neighborCoord = clamp(int3(dispatchThreadId) + kNeighborOffsets[i], 0, maxCoord);
        float4 neighbor = gVoxelInjectCurrent.Load(int4(neighborCoord, 0));

        float difference = abs(centerLuminance - CalculateLuminance(neighbor.rgb)) + abs(center.a - neighbor.a);
        float weight = exp(-difference * sensitivity);

        sum += neighbor * weight;
        totalWeight += weight;
    }

    gVoxelInjectFiltered[dispatchThreadId] = sum / totalWeight;
}
