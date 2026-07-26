struct Particle
{
    float3 position;
    float3 velocity;
    float life;
    float maxLife;
};

// バッファの定義
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<uint> gFreeList : register(u1); 
RWStructuredBuffer<uint> gFreeListCounter : register(u2);

cbuffer EmitterData : register(b0)
{
    float3 gEmitterPos;
    float gDeltaTime;
    float gTime;
    uint gEmitCount;
};

float Random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

[numthreads(64, 1, 1)]
void UpdateCS(uint3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    if (idx >= 10000)
        return;

    Particle p = gParticles[idx];
    
    if (p.life > 0.0f)
    {
        p.life -= gDeltaTime;

        if (p.life <= 0.0f)
        {
            uint freeIdx;
            InterlockedAdd(gFreeListCounter[0], 1, freeIdx);
            gFreeList[freeIdx] = idx;
        }
        else
        {
            // 移動処理
            p.position += p.velocity * gDeltaTime;
        }
        gParticles[idx] = p;
    }
}

[numthreads(64, 1, 1)]
void EmitCS(uint3 dtid : SV_DispatchThreadID)
{
    uint emitIdx = dtid.x;
    if (emitIdx >= gEmitCount)
        return;
    
    uint currentCount;
    InterlockedAdd(gFreeListCounter[0], -1, currentCount);

    if (currentCount > 0)
    {
        uint particleIdx = gFreeList[currentCount - 1]; 

        Particle p;
        p.position = gEmitterPos;
        
        float randX = Random(float2(emitIdx, gTime)) * 2.0f - 1.0f;
        float randY = Random(float2(emitIdx, gTime + 1.0f));
        float randZ = Random(float2(emitIdx, gTime + 2.0f)) * 2.0f - 1.0f;
        
        p.velocity = normalize(float3(randX, randY, randZ)) * 2.0f;
        p.maxLife = 1.0f + Random(float2(emitIdx, gTime + 3.0f)) * 2.0f;
        p.life = p.maxLife;

        gParticles[particleIdx] = p;
    }
    else
    {
        InterlockedAdd(gFreeListCounter[0], 1);
    }
}