struct Particle
{
    float3 position;
    float3 velocity;
    float life;
    float maxLife;
};

// バッファの定義
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<uint> gFreeList : register(u1); // 空きインデックスのスタック
RWStructuredBuffer<uint> gFreeListCounter : register(u2); // 現在の空き要素数

cbuffer EmitterData : register(b0)
{
    float3 gEmitterPos;
    float gDeltaTime;
    float gTime;
    uint gEmitCount; // 今回発生させる数
};

float Random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

// Update (寿命を減らし、死んだらFreeListに返す)
[numthreads(64, 1, 1)]
void UpdateCS(uint3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    if (idx >= 10000)
        return;

    Particle p = gParticles[idx];

    // 生きているパーティクルのみ処理
    if (p.life > 0.0f)
    {
        p.life -= gDeltaTime;

        if (p.life <= 0.0f)
        {
            // 寿命が尽きたら FreeList にインデックスを返却 (Push)
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

// Emit (FreeListから取り出して発生させる)
[numthreads(64, 1, 1)]
void EmitCS(uint3 dtid : SV_DispatchThreadID)
{
    uint emitIdx = dtid.x;
    if (emitIdx >= gEmitCount)
        return;

    // FreeListからインデックスを取得 (Pop)
    uint currentCount;
    InterlockedAdd(gFreeListCounter[0], -1, currentCount);

    if (currentCount > 0)
    {
        uint particleIdx = gFreeList[currentCount - 1]; // 取得したインデックス

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
        // FreeListが空だった場合（発生上限）、カウンタを元に戻す
        InterlockedAdd(gFreeListCounter[0], 1);
    }
}