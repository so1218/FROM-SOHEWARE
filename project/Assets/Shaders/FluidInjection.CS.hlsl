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
    float dist = distance(voxelWorldPos, gFluidSettings.objectPos);
    float influence = smoothstep(gFluidSettings.interactionRadius, 0.0f, dist);

    float3 currentVel = gVelocityRead[DTid.xyz].xyz;
    float currentDen = gDensityRead[DTid.xyz];

    if (influence > 0.0f)
    {
        // 影響範囲内：キャラクターの動力を強くインジェクション
        float3 addedVel = gFluidSettings.objectVelocity * influence * gFluidSettings.injectionStrength * gFrameData.deltaTime;
        float targetDensity = -gFluidSettings.densityAmount * influence;
        
        // 現在の流体バッファの値にブレンド/加算
        float newDen = lerp(currentDen, targetDensity, influence);
        float3 newVel = currentVel + addedVel;

        gVelocityWrite[DTid.xyz] = float4(newVel, 0.0f);
        gDensityWrite[DTid.xyz] = newDen;
    }
    else
    {
        // ★影響範囲外：何もしない！（アドベクションと減衰パスに完全に任せる）
        gVelocityWrite[DTid.xyz] = float4(currentVel, 0.0f);
        gDensityWrite[DTid.xyz] = currentDen;
    }
}