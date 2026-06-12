#include "ShaderConstants.hlsli"

Texture3D<float4> gVelocityRead : register(t0);
Texture3D<float> gPressure : register(t1);
// u0: 書き込み先速度場（サイズ2に合わせるため、ダミーがu1にバインドされますがHLSL側はu0のみ使用でOK）
RWTexture3D<float4> gVelocityWrite : register(u0);

ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocityWrite.GetDimensions(width, height, depth);
    if (any(DTid >= uint3(width, height, depth)))
        return;

    float pCenter = gPressure[DTid];
    float pL = (DTid.x == 0) ? pCenter : gPressure[DTid - uint3(1, 0, 0)];
    float pR = (DTid.x == width - 1) ? pCenter : gPressure[DTid + uint3(1, 0, 0)];
    float pB = (DTid.y == 0) ? pCenter : gPressure[DTid - uint3(0, 1, 0)];
    float pT = (DTid.y == height - 1) ? pCenter : gPressure[DTid + uint3(0, 1, 0)];
    float pD = (DTid.z == 0) ? pCenter : gPressure[DTid - uint3(0, 0, 1)];
    float pU = (DTid.z == depth - 1) ? pCenter : gPressure[DTid + uint3(0, 0, 1)];

    float halfInvDx = 0.5f / gFluidSettings.gridScale;
    float3 gradient = halfInvDx * float3(pR - pL, pT - pB, pU - pD);

    float3 currentVel = gVelocityRead[DTid.xyz].xyz;
    
    gVelocityWrite[DTid.xyz] = float4(currentVel - gradient, 0.0f);
}