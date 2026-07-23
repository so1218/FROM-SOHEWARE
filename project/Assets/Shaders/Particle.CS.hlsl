struct Particle
{
    float3 position;
    float3 velocity;
    float life; // 現在の寿命
    float maxLife; // 最大寿命
};

// 読み書き可能なパーティクルバッファ
RWStructuredBuffer<Particle> gParticles : register(u0);

// C++から毎フレーム送る定数バッファ
cbuffer EmitterData : register(b0)
{
    float3 gEmitterPos; // パーティクルの発生源
    float gDeltaTime; // フレーム間の時間
    float gTime; // 乱数用の時間シード
};

// 簡易的な疑似乱数生成関数
float Random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    
    // 配列外アクセス防止（パーティクル最大数に合わせる。ここでは例として10000）
    if (idx >= 10000)
        return;

    Particle p = gParticles[idx];

    // 1. 寿命を減らす
    p.life -= gDeltaTime;

    // 2. 寿命が尽きたら「リサイクル（再生成）」する
    if (p.life <= 0.0f)
    {
        p.position = gEmitterPos; // 発生源に戻す
        
        // 乱数を使って適当に散らす
        float randX = Random(float2(idx, gTime)) * 2.0f - 1.0f;
        float randY = Random(float2(idx, gTime + 1.0f)); // 上方向に飛ばす
        float randZ = Random(float2(idx, gTime + 2.0f)) * 2.0f - 1.0f;
        
        p.velocity = normalize(float3(randX, randY, randZ)) * 2.0f;
        
        p.maxLife = 1.0f + Random(float2(idx, gTime + 3.0f)) * 2.0f; // 寿命1~3秒
        p.life = p.maxLife;
    }

    // 3. 移動（位置の更新）
    p.position += p.velocity * gDeltaTime;

    // 結果をバッファに書き戻す
    gParticles[idx] = p;
}