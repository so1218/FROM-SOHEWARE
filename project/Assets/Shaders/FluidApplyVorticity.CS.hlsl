#include "ShaderConstants.hlsli"

Texture3D<float4> gVelocityRead : register(t0);
Texture3D<float> gDensityRead : register(t1);
Texture3D<float4> gCurlRead : register(t2); // パス1で作ったCurlテクスチャを入力
RWTexture3D<float4> gVelocityWrite : register(u0);

ConstantBuffer<FrameData> gFrameData : register(b0);
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
    gVelocityWrite.GetDimensions(width, height, depth);
    int3 size = int3(width, height, depth);
    
    if (any(DTid >= uint3(size)))
        return;

    int3 pos = int3(DTid);
    float halfInvDx = 0.5f / gFluidSettings.gridScale;

    // 自身のテクセルのCurlベクトルを取得（RGB成分）
    float3 centerCurl = gCurlRead[pos].xyz;

    // 周囲6マスのCurlの大きさを事前計算テクスチャから取得（W成分）
    float magL = gCurlRead[Wrap(pos + int3(-1, 0, 0), size)].w;
    float magR = gCurlRead[Wrap(pos + int3(1, 0, 0), size)].w;
    float magB = gCurlRead[Wrap(pos + int3(0, -1, 0), size)].w;
    float magT = gCurlRead[Wrap(pos + int3(0, 1, 0), size)].w;
    float magD = gCurlRead[Wrap(pos + int3(0, 0, -1), size)].w;
    float magU = gCurlRead[Wrap(pos + int3(0, 0, 1), size)].w;

    // 渦の強さの勾配を作る
    float3 N = float3(magR - magL, magT - magB, magU - magD) * halfInvDx;
    float lenN = length(N);
    
    // ゼロ除算を防止しつつ正規化
    N = lenN > 0.0001f ? (N / lenN) : float3(0.0f, 0.0f, 0.0f);

    // Vorticity Confinementの力を計算（N方向 × Curlベクトル）
    float3 vorticityForce = cross(N, centerCurl) * gFluidSettings.vorticityStrength;

    // 密度の濃い部分を中心に乱気流を発生させるためのマスク
    float densityMask = saturate(gDensityRead[pos].r * 2.0f);
    vorticityForce *= densityMask;
 
    // 速度の更新と書き込み
    float3 currentVel = gVelocityRead[pos].xyz;
    float3 newVel = currentVel + (vorticityForce * gFrameData.deltaTime);
    
    gVelocityWrite[DTid] = float4(newVel, 0.0f);
}