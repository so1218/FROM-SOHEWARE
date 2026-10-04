#include "Common/Trail.hlsli"
#include "Common/MathUtils.hlsli"

TrailVSOutput main(TrailVSInput input)
{
    TrailVSOutput output;

    // UVスクロール
    output.texcoord = input.texcoord;
    float3 pos = input.position.xyz;
    
    float time = gFrameData.gTime;

    // ジッター適用
    if (gTrailMaterial.jitterStrength > 0.0f)
    {
        float u = input.texcoord.x + gTrailMaterial.jitterPhase + gTrailMaterial.instanceSeed;
        float timeOffset = time * gTrailMaterial.jitterSpeed;
        float3 offset = float3(0.0f, 0.0f, 0.0f);

        if (gTrailMaterial.jitterMode == 0) // Smooth 
        {
            offset.x = sin((u + timeOffset) * gTrailMaterial.jitterFrequency);
            offset.y = cos((u + timeOffset * 1.2f) * gTrailMaterial.jitterFrequency);
            offset.z = sin((u + timeOffset * 0.8f) * gTrailMaterial.jitterFrequency);
        }
        else // Random 
        {
            float timeStep = floor(time * gTrailMaterial.jitterSpeed);
            float baseSeed = timeStep * 13.0f + (gTrailMaterial.instanceSeed * 100.0f);
            float noisePos = input.texcoord.x * gTrailMaterial.jitterFrequency;
            
            float3 noise = ValueNoise31(noisePos + baseSeed);
            float3 sharpNoise = abs(ValueNoise31(noisePos * 2.5f - baseSeed * 1.5f)) * 2.0f - 1.0f;
            
            offset = noise * 0.7f + sharpNoise * 0.3f;

            // 端を固定するエンベロープ
            float pinEnvelope = sin(input.texcoord.x * PI);
            offset *= pinEnvelope;
        }

        pos += offset * gTrailMaterial.jitterStrength;
    }

    // ワールド→ビュー→投影変換
    output.position = mul(float4(pos, 1.0), gTransformationMatrix.WVP);
    output.color = input.color;

    return output;
}