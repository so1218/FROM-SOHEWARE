#include "ShaderConstants.hlsli"

Texture3D<float4> gVelocityRead : register(t0);
Texture3D<float> gDensityRead : register(t1);
Texture3D<float4> gNoiseVolume : register(t2);

RWTexture3D<float4> gVelocityWrite : register(u0);
RWTexture3D<float> gDensityWrite : register(u1);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

SamplerState gLinearWrapSampler : register(s1);

// 3Dノイズテクスチャの有限差分から発散ゼロの渦ベクトルを生成する関数
float3 SampleCurlNoise(SamplerState texSampler, float3 uvw)
{
    // 有限差分のステップ幅
    const float delta = 0.015625f;
    
    // 各軸を微小にずらしてサンプリングし、空間の傾き（勾配）を調べる
    float3 pX = gNoiseVolume.SampleLevel(texSampler, uvw + float3(delta, 0.0f, 0.0f), 0).rgb;
    float3 nX = gNoiseVolume.SampleLevel(texSampler, uvw - float3(delta, 0.0f, 0.0f), 0).rgb;
    
    float3 pY = gNoiseVolume.SampleLevel(texSampler, uvw + float3(0.0f, delta, 0.0f), 0).rgb;
    float3 nY = gNoiseVolume.SampleLevel(texSampler, uvw - float3(0.0f, delta, 0.0f), 0).rgb;
    
    float3 pZ = gNoiseVolume.SampleLevel(texSampler, uvw + float3(0.0f, 0.0f, delta), 0).rgb;
    float3 nZ = gNoiseVolume.SampleLevel(texSampler, uvw - float3(0.0f, 0.0f, delta), 0).rgb;
    
    // 回転の数式を解く
    float3 curl;
    curl.x = (pY.z - nY.z) - (pZ.y - nZ.y);
    curl.y = (pZ.x - nZ.x) - (pX.z - nX.z);
    curl.z = (pX.y - nX.y) - (pY.x - nY.x);
    
    // 微小差分を実用的な速度ベクトルのスケールに調整
    return curl * (0.5f / delta);
}

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
        // Forceではなく、目標となる速度を作る
        float3 dragVelocity = gFluidSettings.objectVelocity * gFluidSettings.dragStrength;
        float speed = length(gFluidSettings.objectVelocity);
        float3 pushVelocity = outwardDir * speed * gFluidSettings.pushStrength;
        
        float3 targetVel = dragVelocity + pushVelocity;

        // 空間座標と時間からCurl Noiseをサンプリング

        float noiseScale = 0.5f;
        float3 noiseUVW = voxelWorldPos * noiseScale + gFrameData.gTime * 0.2f;
        float3 curlNoiseVel = SampleCurlNoise(gLinearWrapSampler, noiseUVW); 

        // キャラクターが動いた時だけ、その周囲に微細な乱気流を発生
        targetVel += curlNoiseVel * (speed * 0.5f);

        float blendRate = influence * saturate(gFrameData.deltaTime * 60.0f);
        gVelocityWrite[DTid.xyz] = float4(lerp(currentVel, targetVel, blendRate), 0.0f);

        // 影響範囲内でも、元々あった密度を必ず維持して書き込む
        gDensityWrite[DTid.xyz] = currentDen;
    }
    else
    {
        gVelocityWrite[DTid.xyz] = float4(currentVel, 0.0f);
        gDensityWrite[DTid.xyz] = currentDen;
    }
}