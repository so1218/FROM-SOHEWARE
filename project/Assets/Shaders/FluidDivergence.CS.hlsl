#include "ShaderConstants.hlsli"

Texture3D<float4> gVelocity : register(t0);
RWTexture3D<float> gDivergence : register(u0);

ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocity.GetDimensions(width, height, depth);
    if (any(DTid >= uint3(width, height, depth)))
        return;

    // Clampではなくループ(Wrap)
    uint xL = (DTid.x == 0) ? width - 1 : DTid.x - 1;
    uint xR = (DTid.x == width - 1) ? 0 : DTid.x + 1;
    uint yB = (DTid.y == 0) ? height - 1 : DTid.y - 1;
    uint yT = (DTid.y == height - 1) ? 0 : DTid.y + 1;
    uint zD = (DTid.z == 0) ? depth - 1 : DTid.z - 1;
    uint zU = (DTid.z == depth - 1) ? 0 : DTid.z + 1;

    // 隣り合うボクセルの速度をループ空間で取得
    float3 vL = gVelocity[uint3(xL, DTid.y, DTid.z)].xyz;
    float3 vR = gVelocity[uint3(xR, DTid.y, DTid.z)].xyz;
    float3 vB = gVelocity[uint3(DTid.x, yB, DTid.z)].xyz;
    float3 vT = gVelocity[uint3(DTid.x, yT, DTid.z)].xyz;
    float3 vD = gVelocity[uint3(DTid.x, DTid.y, zD)].xyz;
    float3 vU = gVelocity[uint3(DTid.x, DTid.y, zU)].xyz;

    float halfInvDx = 0.5f / gFluidSettings.gridScale;
    float divergence = halfInvDx * ((vR.x - vL.x) + (vT.y - vB.y) + (vU.z - vD.z));
    
    gDivergence[DTid] = divergence;
}