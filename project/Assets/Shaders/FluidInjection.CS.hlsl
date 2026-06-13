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
    float3 fluidSize = gFluidSettings.gridMax - gFluidSettings.gridMin;

    float3 rawWorldPos = uvw * fluidSize;
    float3 boxOffset = floor((gFluidSettings.gridMax - rawWorldPos) / fluidSize) * fluidSize;
    float3 voxelWorldPos = rawWorldPos + boxOffset;

    float3 outwardVector = voxelWorldPos - gFluidSettings.objectPos;
    float dist = length(outwardVector);
    float3 outwardDir = dist > 0.001f ? (outwardVector / dist) : float3(0.0f, 1.0f, 0.0f);
    
    float influence = smoothstep(gFluidSettings.interactionRadius, 0.0f, dist);

    float3 currentVel = gVelocityRead[DTid.xyz].xyz;
    float currentDen = gDensityRead[DTid.xyz];

    if (influence > 0.0f)
    {
        // 【Niagara Fluid方式】: Force(加算)ではなく、目標となる速度(Target)を作る
        float3 dragVelocity = gFluidSettings.objectVelocity * gFluidSettings.dragStrength;
        float speed = length(gFluidSettings.objectVelocity);
        float3 pushVelocity = outwardDir * speed * gFluidSettings.pushStrength;
        
        float3 targetVel = dragVelocity + pushVelocity;

        // ★AAAハック: キャラクターの移動に合わせて「渦（Vorticity）」を直接ブレンドする
        // 擬似的なカールノイズのように、外側に向かうベクトルを少し回転させる
        float3 curlComponent = cross(outwardDir, float3(0.0f, 1.0f, 0.0f)) * speed * gFluidSettings.vorticityStrength;
        targetVel += curlComponent;

        // deltaTimeを掛けずに、influenceの強さで直接速度を「補間（上書き）」する！
        // これにより、圧ソルバに消される前に1フレームで完璧にキャラに追従します
        float blendRate = influence * saturate(gFrameData.deltaTime * 60.0f); // 60fps基準
        gVelocityWrite[DTid.xyz] = float4(lerp(currentVel, targetVel, blendRate), 0.0f);

        // 【バグ修正】: 影響範囲内でも、元々あった密度を必ず維持して書き込む！
        gDensityWrite[DTid.xyz] = currentDen;
    }
    else
    {
        gVelocityWrite[DTid.xyz] = float4(currentVel, 0.0f);
        gDensityWrite[DTid.xyz] = currentDen;
    }
}