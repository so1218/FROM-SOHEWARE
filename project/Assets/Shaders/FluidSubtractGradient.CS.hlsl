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

    // ループ(Wrap)座標の計算
    uint xL = (DTid.x == 0) ? width - 1 : DTid.x - 1;
    uint xR = (DTid.x == width - 1) ? 0 : DTid.x + 1;
    uint yB = (DTid.y == 0) ? height - 1 : DTid.y - 1;
    uint yT = (DTid.y == height - 1) ? 0 : DTid.y + 1;
    uint zD = (DTid.z == 0) ? depth - 1 : DTid.z - 1;
    uint zU = (DTid.z == depth - 1) ? 0 : DTid.z + 1;

    // 反対側の圧力を取得
    float pL = gPressure[uint3(xL, DTid.y, DTid.z)];
    float pR = gPressure[uint3(xR, DTid.y, DTid.z)];
    float pB = gPressure[uint3(DTid.x, yB, DTid.z)];
    float pT = gPressure[uint3(DTid.x, yT, DTid.z)];
    float pD = gPressure[uint3(DTid.x, DTid.y, zD)];
    float pU = gPressure[uint3(DTid.x, DTid.y, zU)];

    float halfInvDx = 0.5f / gFluidSettings.gridScale;
    float3 gradient = halfInvDx * float3(pR - pL, pT - pB, pU - pD);

    float3 currentVel = gVelocityRead[DTid].xyz;
    
    gVelocityWrite[DTid] = float4(currentVel - gradient, 0.0f);
}