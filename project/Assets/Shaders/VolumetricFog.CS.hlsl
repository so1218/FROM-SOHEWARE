#include "ShaderConstants.hlsli"
#include "Object3D.hlsli"

Texture2D<float> gDepthTexture : register(t0);
Texture2D<float> gShadowMap : register(t1);
Texture3D<float> gNoiseVolume : register(t2);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

// 出力リソース
RWTexture2D<float4> gOutput : register(u0);

// 定数バッファ
ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

cbuffer PointLights : register(b3)
{
    PointLight gPointLights[MAX_POINT_LIGHTS];
};
cbuffer SpotLights : register(b4)
{
    SpotLight gSpotLights[MAX_SPOT_LIGHTS];
};

// Henyey-Greenstein 位相関数
float PhaseFunctionHG(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * 3.14159265f * pow(denom, 1.5f));
}

float DualPhaseHG(float cosTheta, float g)
{
    // 前方散乱（太陽方向の強い光）
    float forward = PhaseFunctionHG(cosTheta, g);
    // 後方散乱（光源の反対側を向いたときに見える、わずかな反射）
    float backward = PhaseFunctionHG(cosTheta, -0.2f); 
    
    // 9:1 くらいの割合で合成する
    return lerp(backward, forward, 0.9f);
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutput.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    float2 uv = (float2(DTid.xy) + 0.5f) / float2(width, height);
    float depthVal = gDepthTexture.SampleLevel(gSampler, uv, 0).r;

    float clipX = uv.x * 2.0f - 1.0f;
    float clipY = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(clipX, clipY, depthVal, 1.0f);
    float4 worldPos = mul(clipPos, gFrameData.invViewProj);
    worldPos /= worldPos.w;

    float3 rayVec = worldPos.xyz - gFrameData.cameraWorldPosition;
    float rayLength = length(rayVec);
    float3 rayDir = rayVec / max(rayLength, 0.0001f);

    float stepSize = gFogSettings.maxDistance / max((float) gFogSettings.steps, 1.0f);
    float marchLength = min(rayLength, gFogSettings.maxDistance);
    int actualSteps = min(gFogSettings.steps, (int) ceil(marchLength / stepSize));
    
    float dither = frac(52.9829189f * frac(dot(DTid.xy, float2(0.06711056f, 0.00583715f))));
    float3 currentPos = gFrameData.cameraWorldPosition + (rayDir * (stepSize * dither));

    float3 volumetricIllumination = float3(0, 0, 0);
    float3 lightDir = normalize(-gFrameData.mainLightDirection);
    float cosTheta = dot(rayDir, lightDir);

    float phasePhysical = DualPhaseHG(cosTheta, gFogSettings.scatteringG);
    float phaseBase = gFogSettings.phaseBase;
    
    float transmittance = 1.0f;

    for (int i = 0; i < actualSteps; ++i)
    {
        // --- 1. シャドウの計算 ---
        float4 shadowCoord = mul(float4(currentPos, 1.0f), gFrameData.lightViewProj);
        shadowCoord.xyz /= shadowCoord.w;
        float2 shadowUV = shadowCoord.xy * float2(0.5f, -0.5f) + 0.5f;
        float shadowVisibility = 1.0f;
        if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f && shadowUV.y >= 0.0f && shadowUV.y <= 1.0f && shadowCoord.z >= 0.0f && shadowCoord.z <= 1.0f)
        {
            float compareDepth = shadowCoord.z - 0.001f;
            shadowVisibility = gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV, compareDepth);
        }
        
        float objRadius = max(gFogSettings.objectRadius, 0.001f);
        float3 vecToPos = currentPos - gFogSettings.objectPos;
        float distToObj = length(vecToPos);
        float3 dirToPos = vecToPos / (distToObj + 0.0001f);
        
        float speed = length(gFogSettings.objectVelocity);
        float3 velDir = speed > 0.0001f ? (gFogSettings.objectVelocity / speed) : float3(0, 1.0f, 0);
        
        float pushFactor = smoothstep(objRadius * 2.0f, 0.0f, distToObj);
        float3 pushWarp = dirToPos * (pushFactor * objRadius * 1.5f);

        float3 swirlAxis = normalize(cross(velDir, dirToPos) + float3(0.001f, 0.001f, 0.001f));
        float swirlFactor = pushFactor * speed * 0.8f;
        float3 swirlWarp = swirlAxis * swirlFactor;
        
        float distAlongWake = dot(vecToPos, -velDir);
        float distFromWakeCenter = length(vecToPos - (-velDir * distAlongWake));
        
        float wakeLength = max(speed * 3.0f, objRadius * 2.0f);
        float wakeFactor = smoothstep(wakeLength, 0.0f, max(distAlongWake, 0.0f))
                         * smoothstep(objRadius * 1.5f, 0.0f, distFromWakeCenter);
        
        float3 wakeWarp = velDir * (wakeFactor * speed * gFogSettings.interactionPower);
        
        float3 noiseSamplePos = currentPos + pushWarp + swirlWarp - wakeWarp;
        
        float coreMask = smoothstep(objRadius * 0.4f, objRadius * 0.9f, distToObj);

        
        float heightFalloff = exp(-max(currentPos.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);
        float3 timeOffset = float3(1.0f, 0.5f, 0.8f) * (gFrameData.gTime * gFogSettings.windSpeed);
        
        float3 warpUVW = noiseSamplePos * (gFogSettings.noiseScale * 0.5f) + timeOffset * 0.5f;
        float dx = gNoiseVolume.SampleLevel(gSampler, frac(warpUVW), 0);
        float dy = gNoiseVolume.SampleLevel(gSampler, frac(warpUVW + 0.33f), 0);
        float dz = gNoiseVolume.SampleLevel(gSampler, frac(warpUVW + 0.67f), 0);
        float3 distortion = float3(dx, dy, dz);
        
        float3 distortedPos = noiseSamplePos + (distortion * 2.0f - 1.0f) * (0.2f + wakeFactor * 0.5f);
        
        float3 uvw1 = distortedPos * gFogSettings.noiseScale + timeOffset;
        float noise1 = gNoiseVolume.SampleLevel(gSampler, frac(uvw1), 0).r;
        float3 uvw2 = distortedPos * (gFogSettings.noiseScale * 3.0f) - timeOffset * 0.8f;
        float noise2 = gNoiseVolume.SampleLevel(gSampler, frac(uvw2), 0).r;

        float combinedNoise = saturate(noise1 - (1.0f - noise2) * 0.3f);
        float noiseVal = smoothstep(gFogSettings.noiseThreshold, gFogSettings.noiseThreshold + 0.15f, combinedNoise);

        float fogDensity = (gFogSettings.baseAirDensity + (gFogSettings.density * 5.0f * noiseVal)) * heightFalloff;
        
        fogDensity *= coreMask;

        float stepAttenuation = exp(-fogDensity * stepSize);
        
        float directScattering = fogDensity * 50.0f;
        float ambientScattering = fogDensity;
        float3 sunColor = gFrameData.mainLightColor.rgb;
        float3 stepDirect = shadowVisibility * (phasePhysical + phaseBase) * sunColor * directScattering;
        
        float ambientOcclusion = lerp(0.4f, 1.0f, shadowVisibility);
        float3 ambientColor = gFogSettings.fogColor * gFogSettings.ambientFactor * ambientOcclusion;
        float3 stepAmbient = ambientColor * ambientScattering;
        
        float3 stepLocal = float3(0, 0, 0);

        for (int p = 0; p < MAX_POINT_LIGHTS; ++p)
        {
            if (gPointLights[p].enable == 0)
                continue;
            float3 lightVec = gPointLights[p].position - currentPos;
            float dist = length(lightVec);
            if (dist > gPointLights[p].radius)
                continue;

            float3 lDir = lightVec / dist;
            float attenuation = pow(saturate(1.0f - (dist / gPointLights[p].radius)), gPointLights[p].decay);
            float phaseLocal = DualPhaseHG(dot(rayDir, lDir), 0.3f);
            
            stepLocal += gPointLights[p].color.rgb * gPointLights[p].intensity * attenuation * phaseLocal * directScattering;
        }
        
        for (int s = 0; s < MAX_SPOT_LIGHTS; ++s)
        {
            if (gSpotLights[s].enable == 0)
                continue;
            float3 lightVec = gSpotLights[s].position - currentPos;
            float dist = length(lightVec);
            if (dist > gSpotLights[s].distance)
                continue;

            float3 lDir = lightVec / dist;
            float currentCos = dot(-lDir, normalize(gSpotLights[s].direction));
            if (currentCos < gSpotLights[s].cosAngle)
                continue;

            float angleFalloff = pow(saturate((currentCos - gSpotLights[s].cosAngle) / (1.0f - gSpotLights[s].cosAngle)), 2.0f);
            float distFalloff = pow(saturate(1.0f - (dist / gSpotLights[s].distance)), gSpotLights[s].decay);
            float phaseLocal = DualPhaseHG(dot(rayDir, lDir), 0.3f);

            stepLocal += gSpotLights[s].color.rgb * gSpotLights[s].intensity * angleFalloff * distFalloff * phaseLocal * directScattering;
        }

        // 光の散乱の総量
        float3 S = stepDirect + stepAmbient + stepLocal;
        float3 stepScattering = S * ((1.0f - stepAttenuation) / max(fogDensity, 0.00001f));

        volumetricIllumination += stepScattering * transmittance;
        transmittance *= stepAttenuation;

        currentPos += rayDir * stepSize;
    }
    
    volumetricIllumination *= gFogSettings.intensity;
    gOutput[DTid.xy] = float4(volumetricIllumination, transmittance);
}