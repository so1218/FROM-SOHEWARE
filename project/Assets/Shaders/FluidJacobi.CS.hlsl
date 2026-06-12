#include "ShaderConstants.hlsli"

Texture3D<float> gPressureRead : register(t0);
Texture3D<float> gDivergence : register(t1);
RWTexture3D<float> gPressureWrite : register(u0);

ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gPressureWrite.GetDimensions(width, height, depth);
    if (any(DTid >= uint3(width, height, depth)))
        return;

    // ★修正：ノイマン境界条件。壁の外は「自分自身の圧力」として扱う
    float pCenter = gPressureRead[DTid];
    float pL = (DTid.x == 0) ? pCenter : gPressureRead[DTid - uint3(1, 0, 0)];
    float pR = (DTid.x == width - 1) ? pCenter : gPressureRead[DTid + uint3(1, 0, 0)];
    float pB = (DTid.y == 0) ? pCenter : gPressureRead[DTid - uint3(0, 1, 0)];
    float pT = (DTid.y == height - 1) ? pCenter : gPressureRead[DTid + uint3(0, 1, 0)];
    float pD = (DTid.z == 0) ? pCenter : gPressureRead[DTid - uint3(0, 0, 1)];
    float pU = (DTid.z == depth - 1) ? pCenter : gPressureRead[DTid + uint3(0, 0, 1)];

    float div = gDivergence[DTid];
    float dxSq = gFluidSettings.gridScale * gFluidSettings.gridScale;
    
    float newPressure = (pL + pR + pB + pT + pD + pU - div * dxSq) / 6.0f;

    gPressureWrite[DTid] = newPressure;
}