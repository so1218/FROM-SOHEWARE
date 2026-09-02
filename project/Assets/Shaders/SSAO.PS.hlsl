#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli" 

ConstantBuffer<SSAOSettings> gSSAOSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

Texture2D<float4> gNormalTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);

SamplerState gClampSampler : register(s0);

float LinearizeDepth(float depth, float nearClip, float farClip)
{
    return (nearClip * farClip) / (farClip - depth * (farClip - nearClip));
}

float3 GetViewPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(x, y, depth, 1.0f);
    float4 viewPos = mul(clipPos, gFrameData.invProjMatrix);
    return viewPos.xyz / viewPos.w;
}

float InterleavedGradientNoise(float2 pixelPos)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelPos, magic.xy)));
}

float4 main(VSOutput input) : SV_TARGET
{
    float depth = gDepthTexture.SampleLevel(gClampSampler, input.uv, 0);
    
    if (depth >= 1.0f)
        return float4(1.0f, 1.0f, 1.0f, 1.0f);

    float3 worldNormal = gNormalTexture.SampleLevel(gClampSampler, input.uv, 0).xyz;
    float3 viewPos = GetViewPos(input.uv, depth);
    float3 viewNormal = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));

    // NaNを起こさない安全なTBN行列の構築
    float3 upVector = abs(viewNormal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(upVector, viewNormal));
    float3 bitangent = cross(viewNormal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, viewNormal);

    // ノイズによるランダムな回転角度だけを取得
    float noise = InterleavedGradientNoise(input.position.xy);
    float randomAngle = noise * 2.0f * 3.14159265f;

    float occlusion = 0.0f;
    int sampleCount = gSSAOSettings.sampleCount;

    [unroll(32)] // サンプル数が固定ならばアンロールして高速化
    for (int i = 0; i < sampleCount; ++i)
    {
        float u = (float(i) + 0.5f) / float(sampleCount);
        // スパイラルにランダム角度を足すことで全体を回転
        float theta = u * 2.0f * 3.14159265f * 7.0f + randomAngle;
        
        float r = sqrt(u);
        float z = sqrt(max(0.0f, 1.0f - r * r));
        float3 hemispherePos = float3(r * cos(theta), r * sin(theta), z);

        float3 sampleOffset = mul(hemispherePos, TBN);
        sampleOffset *= lerp(0.1f, 1.0f, u * u);

        float3 samplePos = viewPos + sampleOffset * gSSAOSettings.radius;

        float4 offsetPos = mul(float4(samplePos, 1.0f), gFrameData.projectionMatrix);
        offsetPos.xyz /= offsetPos.w;
        float2 sampleUV = float2(offsetPos.x * 0.5f + 0.5f, 1.0f - (offsetPos.y * 0.5f + 0.5f));

        if (sampleUV.x < 0.0f || sampleUV.x > 1.0f || sampleUV.y < 0.0f || sampleUV.y > 1.0f)
            continue;

        float sampleDepth = gDepthTexture.SampleLevel(gClampSampler, sampleUV, 0);
        float sampleZ = LinearizeDepth(sampleDepth, gFrameData.nearClip, gFrameData.farClip);

        // 距離に基づく正確な減衰（Haloアーティファクトの防止）
        float distance = abs(viewPos.z - sampleZ);
        float rangeCheck = smoothstep(0.0f, 1.0f, 1.0f - saturate(distance / gSSAOSettings.radius));
        
        if (sampleZ < samplePos.z - gSSAOSettings.bias)
        {
            occlusion += 1.0f * rangeCheck;
        }
    }

    occlusion = 1.0f - (occlusion / (float) sampleCount);
    
    // 遠距離フェード
    float linearDepth = LinearizeDepth(depth, gFrameData.nearClip, gFrameData.farClip);
    float fade = saturate((linearDepth - gSSAOSettings.fadeStart) / (gSSAOSettings.fadeEnd - gSSAOSettings.fadeStart));
    occlusion = lerp(occlusion, 1.0f, fade);

    // コントラスト調整
    occlusion = pow(abs(occlusion), gSSAOSettings.intensity);

    return float4(occlusion, occlusion, occlusion, 1.0f);
}