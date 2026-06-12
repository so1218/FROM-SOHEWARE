#include "ShaderConstants.hlsli"

Texture3D<float4> gVelocityRead : register(t0);
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

    float halfInvDx = 0.5f / gFluidSettings.gridScale;

    // 1. 上下左右前後の速度を取得
    float3 vL = gVelocityRead[max(DTid - uint3(1, 0, 0), 0)].xyz;
    float3 vR = gVelocityRead[min(DTid + uint3(1, 0, 0), uint3(width - 1, height - 1, depth - 1))].xyz;
    float3 vB = gVelocityRead[max(DTid - uint3(0, 1, 0), 0)].xyz;
    float3 vT = gVelocityRead[min(DTid + uint3(0, 1, 0), uint3(width - 1, height - 1, depth - 1))].xyz;
    float3 vD = gVelocityRead[max(DTid - uint3(0, 0, 1), 0)].xyz;
    float3 vU = gVelocityRead[min(DTid + uint3(0, 0, 1), uint3(width - 1, height - 1, depth - 1))].xyz;

    // 2. 速度場の回転（Curl / Vorticity）を計算
    // カールはベクトル場における「渦の強さと回転軸」を表します
    float3 curl;
    curl.x = ((vT.z - vB.z) - (vU.y - vD.y)) * halfInvDx;
    curl.y = ((vU.x - vD.x) - (vR.z - vL.z)) * halfInvDx;
    curl.z = ((vR.y - vL.y) - (vT.x - vB.x)) * halfInvDx;

    // 渦の強さ（大きさ）
    float curlMag = length(curl);

    // =======================================================
    // ※厳密にはここで隣接ボクセルの「curlMag」を取得して勾配(Gradient)を
    // 計算しますが、リアルタイム向けの高速化として、近似的な力場を適用します
    // =======================================================
    
    // 現在の速度を取得
    float3 currentVel = gVelocityRead[DTid].xyz;

    // 渦の回転軸に対して垂直な方向にエネルギー（速度）を再注入する
    // gFluidSettings.vorticityStrength は 0.1 ～ 2.0 程度で調整
    float3 vorticityForce = cross(curl, currentVel) * gFluidSettings.vorticityStrength;
     
    // 新しい速度として書き込み（密度の更新は不要なのでVelocityのみ）
    float3 newVel = currentVel + (vorticityForce * gFrameData.deltaTime);
    
    gVelocityWrite[DTid] = float4(newVel, 0.0f);
}