#include "ShaderConstants.hlsli"

Texture3D<float4> gVelocityRead : register(t0);
RWTexture3D<float4> gCurlWrite : register(u0);

ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

// Toroidal Wrap関数
uint3 Wrap(int3 p, int3 size)
{
    return uint3(
        p.x < 0 ? size.x - 1 : (p.x >= size.x ? 0 : p.x),
        p.y < 0 ? size.y - 1 : (p.y >= size.y ? 0 : p.y),
        p.z < 0 ? size.z - 1 : (p.z >= size.z ? 0 : p.z)
    );
}

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gCurlWrite.GetDimensions(width, height, depth);
    int3 size = int3(width, height, depth);
    
    if (any(DTid >= uint3(size)))
        return;

    int3 pos = int3(DTid);
    float halfInvDx = 0.5f / gFluidSettings.gridScale;

    // 周囲6マスの速度を読み込む
    float3 vL = gVelocityRead[Wrap(pos + int3(-1, 0, 0), size)].xyz;
    float3 vR = gVelocityRead[Wrap(pos + int3(1, 0, 0), size)].xyz;
    float3 vB = gVelocityRead[Wrap(pos + int3(0, -1, 0), size)].xyz;
    float3 vT = gVelocityRead[Wrap(pos + int3(0, 1, 0), size)].xyz;
    float3 vD = gVelocityRead[Wrap(pos + int3(0, 0, -1), size)].xyz;
    float3 vU = gVelocityRead[Wrap(pos + int3(0, 0, 1), size)].xyz;

    // 渦ベクトルの計算
    float3 curl;
    curl.x = ((vT.z - vB.z) - (vU.y - vD.y)) * halfInvDx;
    curl.y = ((vU.x - vD.x) - (vR.z - vL.z)) * halfInvDx;
    curl.z = ((vR.y - vL.y) - (vT.x - vB.x)) * halfInvDx;

    // Curlの大きさ
    float curlMag = length(curl);

    // RGBに渦ベクトル、Alphaに渦の大きさを保存
    gCurlWrite[DTid] = float4(curl, curlMag);
}