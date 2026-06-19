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

    // ループ(Wrap)座標の計算
    uint xL = (DTid.x == 0) ? width - 1 : DTid.x - 1;
    uint xR = (DTid.x == width - 1) ? 0 : DTid.x + 1;
    uint yB = (DTid.y == 0) ? height - 1 : DTid.y - 1;
    uint yT = (DTid.y == height - 1) ? 0 : DTid.y + 1;
    uint zD = (DTid.z == 0) ? depth - 1 : DTid.z - 1;
    uint zU = (DTid.z == depth - 1) ? 0 : DTid.z + 1;

    // ノイマン境界条件を廃止し、反対側の圧力を取得
    float pL = gPressureRead[uint3(xL, DTid.y, DTid.z)];
    float pR = gPressureRead[uint3(xR, DTid.y, DTid.z)];
    float pB = gPressureRead[uint3(DTid.x, yB, DTid.z)];
    float pT = gPressureRead[uint3(DTid.x, yT, DTid.z)];
    float pD = gPressureRead[uint3(DTid.x, DTid.y, zD)];
    float pU = gPressureRead[uint3(DTid.x, DTid.y, zU)];

    float div = gDivergence[DTid];
    float dxSq = gFluidSettings.gridScale * gFluidSettings.gridScale;
    
    float newPressure = (pL + pR + pB + pT + pD + pU - div * dxSq) / 6.0f;
    
    // 基準点の圧力を常に強制ゼロ、あるいは微小に減衰
    if (all(DTid == uint3(0, 0, 0)))
    {
        newPressure = 0.0f;
    }

    gPressureWrite[DTid] = newPressure;
}