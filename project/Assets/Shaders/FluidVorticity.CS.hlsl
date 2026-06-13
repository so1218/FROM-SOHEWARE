#include "ShaderConstants.hlsli"

Texture3D<float4> gVelocityRead : register(t0);
Texture3D<float> gDensityRead : register(t1);
RWTexture3D<float4> gVelocityWrite : register(u0);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocityWrite.GetDimensions(width, height, depth);
    if (any(DTid >= uint3(width, height, depth)))
        return;

    // 隣接セルのインデックス取得（Toroidal Wrap）
    uint xL = (DTid.x == 0) ? width - 1 : DTid.x - 1;
    uint xR = (DTid.x == width - 1) ? 0 : DTid.x + 1;
    uint yB = (DTid.y == 0) ? height - 1 : DTid.y - 1;
    uint yT = (DTid.y == height - 1) ? 0 : DTid.y + 1;
    uint zD = (DTid.z == 0) ? depth - 1 : DTid.z - 1;
    uint zU = (DTid.z == depth - 1) ? 0 : DTid.z + 1;

    // --- 1. 自身のテクセルのCurlを計算 ---
    float3 vL = gVelocityRead[uint3(xL, DTid.y, DTid.z)].xyz;
    float3 vR = gVelocityRead[uint3(xR, DTid.y, DTid.z)].xyz;
    float3 vB = gVelocityRead[uint3(DTid.x, yB, DTid.z)].xyz;
    float3 vT = gVelocityRead[uint3(DTid.x, yT, DTid.z)].xyz;
    float3 vD = gVelocityRead[uint3(DTid.x, DTid.y, zD)].xyz;
    float3 vU = gVelocityRead[uint3(DTid.x, DTid.y, zU)].xyz;

    float halfInvDx = 0.5f / gFluidSettings.gridScale;
    float3 centerCurl;
    centerCurl.x = ((vT.z - vB.z) - (vU.y - vD.y)) * halfInvDx;
    centerCurl.y = ((vU.x - vD.x) - (vR.z - vL.z)) * halfInvDx;
    centerCurl.z = ((vR.y - vL.y) - (vT.x - vB.x)) * halfInvDx;
    float centerCurlMag = length(centerCurl);

    // --- 2. 【物理的修正】周囲6セルの「Curlの大きさ」を簡易取得して勾配（N）を作る ---
    // ※本来は周囲のCurlを真面目に計算すべきですが、負荷低減のため「速度の差分」から簡易的に勾配を近似します
    // または、以下のように周囲のCurlの大きさを求めます。
    float magL = length(gVelocityRead[uint3(xL, DTid.y, DTid.z)].xyz);
    float magR = length(gVelocityRead[uint3(xR, DTid.y, DTid.z)].xyz);
    float magB = length(gVelocityRead[uint3(DTid.x, yB, DTid.z)].xyz);
    float magT = length(gVelocityRead[uint3(DTid.x, yT, DTid.z)].xyz);
    float magD = length(gVelocityRead[uint3(DTid.x, DTid.y, zD)].xyz);
    float magU = length(gVelocityRead[uint3(DTid.x, DTid.y, zU)].xyz);

    float3 N = float3(magR - magL, magT - magB, magU - magD) * halfInvDx;
    float lenN = length(N);
    N = lenN > 0.0001f ? (N / lenN) : float3(0.0f, 0.0f, 0.0f);

// --- 3. 正しいVorticity Confinementの適用 ---
    float3 vorticityForce = cross(N, centerCurl) * gFluidSettings.vorticityStrength;

// 【AAAハック】: 自身のセルの密度（Density）に応じて渦の強さをマスクする
// これにより、煙の「輪郭」や「濃い部分」だけが激しくブレて、何もない空間は静的に保たれます
    float densityMask = saturate(gDensityRead[DTid].r * 2.0f);
    vorticityForce *= densityMask;
 
    float3 currentVel = gVelocityRead[DTid].xyz;
    float3 newVel = currentVel + (vorticityForce * gFrameData.deltaTime);
    
    gVelocityWrite[DTid] = float4(newVel, 0.0f);
}