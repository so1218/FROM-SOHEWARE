#include "ShaderConstants.hlsli"

// ★修正：ダブルバッファリング用に読み込み(t0, t1)と書き込み(u0, u1)を分ける
Texture3D<float4> gVelocityRead : register(t0);
Texture3D<float> gDensityRead : register(t1);

RWTexture3D<float4> gVelocityWrite : register(u0);
RWTexture3D<float> gDensityWrite : register(u1);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocityWrite.GetDimensions(width, height, depth);

    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    float3 uvw = (float3(DTid) + 0.5f) / float3(width, height, depth);
    float3 voxelWorldPos = lerp(gFluidSettings.gridMin, gFluidSettings.gridMax, uvw);
    
    // キャラクターからボクセルへの方向ベクトルと距離
    float3 outwardVector = voxelWorldPos - gFluidSettings.objectPos;
    float dist = length(outwardVector);
    float3 outwardDir = dist > 0.001f ? (outwardVector / dist) : float3(0.0f, 1.0f, 0.0f);
    
    float influence = smoothstep(gFluidSettings.interactionRadius, 0.0f, dist);

    float3 currentVel = gVelocityRead[DTid.xyz].xyz;
    float currentDen = gDensityRead[DTid.xyz];
    
// =========================================================
    // ★大改造2：押し退ける力（Push）と引きずる力（Drag）の合成
    // =========================================================
    if (influence > 0.0f)
    {
        // 1. Drag (引きずる力) : オブジェクトの移動方向
        float3 dragForce = gFluidSettings.objectVelocity * gFluidSettings.dragStrength;

        // 2. Push (押し退ける力) : オブジェクトの中心から外側へ向かう方向
        // オブジェクトの移動速度の大きさに比例して、空気を押し退ける
        float speed = length(gFluidSettings.objectVelocity);
        float3 pushForce = outwardDir * speed * gFluidSettings.pushStrength;

        // 合成した力を注入
        float3 addedVel = (dragForce + pushForce) * influence * gFrameData.deltaTime;
        
        gVelocityWrite[DTid.xyz] = float4(currentVel + addedVel, 0.0f);

        // 【おまけ】もし「キャラから煙を出したい」ならここでDensityも足す
        // gDensityWrite[DTid.xyz] = currentDen + (influence * 5.0f * gFrameData.deltaTime);
    }
    else
    {
        // 影響範囲外はそのままパススルー（Ping-Pong用）
        gVelocityWrite[DTid.xyz] = float4(currentVel, 0.0f);
        gDensityWrite[DTid.xyz] = currentDen;
    }
}