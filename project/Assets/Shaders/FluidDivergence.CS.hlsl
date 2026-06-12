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

    // 隣り合うボクセルの速度を取得
    float3 vL = gVelocity[max(DTid - uint3(1, 0, 0), 0)].xyz;
    float3 vR = gVelocity[min(DTid + uint3(1, 0, 0), uint3(width - 1, height - 1, depth - 1))].xyz;
    float3 vB = gVelocity[max(DTid - uint3(0, 1, 0), 0)].xyz;
    float3 vT = gVelocity[min(DTid + uint3(0, 1, 0), uint3(width - 1, height - 1, depth - 1))].xyz;
    float3 vD = gVelocity[max(DTid - uint3(0, 0, 1), 0)].xyz;
    float3 vU = gVelocity[min(DTid + uint3(0, 0, 1), uint3(width - 1, height - 1, depth - 1))].xyz;

    // ★高品質化：境界条件（壁の外からは風は吹かない、壁にぶつかった風は止まる）の適用
    if (DTid.x == 0)
        vL.x = -vR.x; // 左の壁：速度を反転（または 0）
    if (DTid.x == width - 1)
        vR.x = -vL.x; // 右の壁
    if (DTid.y == 0)
        vB.y = -vT.y; // 下の壁
    if (DTid.y == height - 1)
        vT.y = -vB.y; // 上の壁
    if (DTid.z == 0)
        vD.z = -vU.z; // 手前の壁
    if (DTid.z == depth - 1)
        vU.z = -vD.z; // 奥の壁

    // ボクセルサイズ(dx)を考慮した発散の計算
    float halfInvDx = 0.5f / gFluidSettings.gridScale;
    float divergence = halfInvDx * ((vR.x - vL.x) + (vT.y - vB.y) + (vU.z - vD.z));
    
    gDivergence[DTid] = divergence;
}