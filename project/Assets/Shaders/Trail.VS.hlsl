#include "Trail.hlsli"

float hash11(float p)
{
    p = frac(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return frac(p);
}

float3 hash31(float p)
{
    float3 p3 = frac(float3(p, p, p) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xxy + p3.yzz) * p3.zyx) * 2.0 - 1.0;
}

float3 valueNoise31(float p)
{
    float i = floor(p);
    float f = frac(p);
    
    f = f * f * (3.0 - 2.0 * f);
    return lerp(hash31(i), hash31(i + 1.0), f);
}

TrailVSOutput main(TrailVSInput input)
{
    TrailVSOutput output;

    // UVスクロール
    float time = gFrameData.gTime;
    float2 scroll = gTrailMaterial.scrollSpeed * time;
    output.texcoord = input.texcoord + scroll;
    output.texcoordRaw = input.texcoord;

    float3 pos = input.position.xyz;

    // ジッター適用
    if (gTrailMaterial.jitterStrength > 0.0)
    {
        // uにinstanceSeedを足して波の開始位置をずらす
        float u = input.texcoord.x + gTrailMaterial.jitterPhase + gTrailMaterial.instanceSeed;
        
        float timeOffset = time * gTrailMaterial.jitterSpeed;
        float3 offset = float3(0, 0, 0);

        if (gTrailMaterial.jitterMode == 0)
        {
            // Wave
            offset.x = sin((u + timeOffset) * gTrailMaterial.jitterFrequency);
            offset.y = cos((u + timeOffset * 1.2) * gTrailMaterial.jitterFrequency);
            offset.z = sin((u + timeOffset * 0.8) * gTrailMaterial.jitterFrequency);
        }
        else if (gTrailMaterial.jitterMode == 1)
        {
            // Digital Wave
            float uStep = floor(u * gTrailMaterial.jitterFrequency);
            float timeStep = floor(time * gTrailMaterial.jitterSpeed);
            float stepInput = uStep + timeStep;

            offset.x = sin(stepInput);
            offset.y = cos(stepInput + stepInput * 0.2);
            offset.z = sin(stepInput - stepInput * 0.2);
        }
        else
        {
            float timeStep = floor(time * gTrailMaterial.jitterSpeed);
            float baseSeed = timeStep * 13.0 + (gTrailMaterial.instanceSeed * 100.0);
 
            float noisePos = input.texcoord.x * gTrailMaterial.jitterFrequency;

            float3 noise = valueNoise31(noisePos + baseSeed);
       
            float3 sharpNoise = abs(valueNoise31(noisePos * 2.5 - baseSeed * 1.5)) * 2.0 - 1.0;
            
            offset = noise * 0.7 + sharpNoise * 0.3;
            
            float pinEnvelope = sin(input.texcoord.x * 3.14159265);
            
            offset *= pinEnvelope;
        }

        pos += offset * gTrailMaterial.jitterStrength;
    }

    // ワールド→ビュー→投影変換
    output.position = mul(float4(pos, 1.0), gTransformationMatrix.WVP);
    output.color = input.color;

    return output;
}