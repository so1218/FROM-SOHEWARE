#include "ParticleCommon.hlsli"

StructuredBuffer<Particle> particleIn : register(t0); // 前フレーム
RWStructuredBuffer<Particle> particleOut : register(u0); // 今フレーム
RWStructuredBuffer<InstanceData> instanceOut : register(u1); // 描画用

cbuffer SimulationParam : register(b0)
{
    float deltaTime;
    float3 gravity;
};

float4x4 CreateBillboardMatrix(float3 position)
{
    return float4x4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        position.x, position.y, position.z, 1
    );
}

[numthreads(256, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint index = dtid.x;

    Particle p = particleIn[index];

    if (p.lifetime <= 0.0f)
    {
        // Emit
        p.position = float3(0.0f, 0.0f, 0.0f);
        p.velocity = float3(0.0f, 1.0f, 0.0f);
        p.lifetime = 3.0f;
        p.maxLifetime = 3.0f;
        p.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
        p.textureIndex = 0;
    }
    else
    {
        p.velocity += gravity * deltaTime;
        p.position += p.velocity * deltaTime;
        p.lifetime -= deltaTime;
    }

    particleOut[index] = p;

    // 描画用データを出力
    InstanceData inst;
    inst.world = CreateBillboardMatrix(p.position);
    inst.color = p.color;
    inst.textureIndex = p.textureIndex;
    instanceOut[index] = inst;
}