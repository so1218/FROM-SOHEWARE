#include "Common/FullScreenQuad.hlsli"
#include "Common/ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0); 
Texture2D gBloomTexture : register(t1); // Bloom用 (光のみボケ)
Texture2D gDoFTexture : register(t2); // DoF用 (全体ボケ)
Texture2D<float> gDepthTexture : register(t3);
Texture2D gVolumetricFogTexture : register(t4);
Texture2D gSSAOTexture : register(t5);

SamplerState gSampler : register(s0);
SamplerState gWrapSampler : register(s1);

ConstantBuffer<FinalCompositeSettings> gCompositeSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// 深度リニア化関数
float LinearizeDepth(float d)
{
    float n = gFrameData.nearClip;
    float f = gFrameData.farClip;

    return (n * f) / (f - d * (f - n));
}

// トーンマッピング
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float CalculateHeightFogOpticalDepth(float y0, float directionY, float rayLength, float density, float baseHeight, float falloff)
{
    if (falloff <= 0.0f)
        return density * rayLength;

    float startDensity = min(exp(-falloff * (y0 - baseHeight)), 1.0f);
    float heightChange = falloff * directionY * rayLength;
    float integral = (abs(heightChange) > 0.0001f)
        ? startDensity * (1.0f - exp(-heightChange)) / (falloff * directionY)
        : startDensity * rayLength;
    
    return density * min(integral, rayLength);
}

float2 IntersectRectangleXZ(float3 origin, float3 direction, float2 rectangleMin, float2 rectangleMax)
{
    float2 safeDirection = select(abs(direction.xz) < 1e-6f, 1e-6f, direction.xz);
    float2 inverseDirection = 1.0f / safeDirection;
    float2 distanceToMin = (rectangleMin - origin.xz) * inverseDirection;
    float2 distanceToMax = (rectangleMax - origin.xz) * inverseDirection;
    float2 nearDistance = min(distanceToMin, distanceToMax);
    float2 farDistance = max(distanceToMin, distanceToMax);
    return float2(max(nearDistance.x, nearDistance.y), min(farDistance.x, farDistance.y));
}

float CalculateBoundaryOpticalDepth(float3 origin, float3 direction, float startDistance, float endDistance)
{
    if (endDistance <= startDistance)
        return 0.0f;

    float expand = gCompositeSettings.boundaryRampLength * 0.5f - gCompositeSettings.boundaryInset;
    float2 inside = IntersectRectangleXZ(origin, direction,
        gCompositeSettings.boundaryMin - expand, gCompositeSettings.boundaryMax + expand);
    bool passesInside = inside.x <= inside.y;
    
    float2 segments[2];
    segments[0] = float2(startDistance, passesInside ? min(endDistance, inside.x) : endDistance);
    segments[1] = passesInside ? float2(max(startDistance, inside.y), endDistance) : float2(0.0f, 0.0f);

    float opticalDepth = 0.0f;
    [unroll]
    for (int index = 0; index < 2; ++index)
    {
        float segmentLength = segments[index].y - segments[index].x;
        if (segmentLength <= 0.0f)
            continue;

        float segmentStartHeight = origin.y + direction.y * segments[index].x;
        opticalDepth += CalculateHeightFogOpticalDepth(segmentStartHeight, direction.y, segmentLength,
            gCompositeSettings.boundaryDensity, gCompositeSettings.boundaryBaseHeight, gCompositeSettings.boundaryHeightFalloff);
    }
    
    float noiseAverage = lerp(1.0f, 0.5f, gCompositeSettings.boundaryNoiseIntensity);
    return opticalDepth * noiseAverage * gCompositeSettings.fogExtinctionScale;
}

float CalculateFogPhase(float cosTheta)
{
    static const float kPi = 3.14159265f;
    float forward = gCompositeSettings.fogAnisotropy;
    float backward = -0.2f;
    float forwardSquared = forward * forward;
    float backwardSquared = backward * backward;
    float forwardPhase = (1.0f - forwardSquared) / (4.0f * kPi * pow(max(1.0f + forwardSquared - 2.0f * forward * cosTheta, 0.0001f), 1.5f));
    float backwardPhase = (1.0f - backwardSquared) / (4.0f * kPi * pow(max(1.0f + backwardSquared - 2.0f * backward * cosTheta, 0.0001f), 1.5f));
    return lerp(backwardPhase, forwardPhase, 0.9f);
}

float3 ApplyBoundaryFog(float3 color, float2 uv, float rawDepth)
{
    if (rawDepth >= 1.0f)
        return color;

    float4 worldPosition = mul(float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, rawDepth, 1.0f), gFrameData.invViewProj);
    float3 toSurface = worldPosition.xyz / worldPosition.w - gFrameData.cameraWorldPosition;
    float surfaceDistance = length(toSurface);
    float3 viewDirection = toSurface / max(surfaceDistance, 0.0001f);
    
    float startDistance = min(gFrameData.farClip, gCompositeSettings.fogMaxDistance) * 0.9f;
    float opticalDepth = CalculateBoundaryOpticalDepth(gFrameData.cameraWorldPosition, viewDirection, startDistance, surfaceDistance);
    if (opticalDepth <= 0.0f)
        return color;

    float transmittance = exp(-opticalDepth);
    
    float selfShadow = exp(-gCompositeSettings.boundaryDensity * gCompositeSettings.fogExtinctionScale * 15.0f);
    float3 toSun = -normalize(gFrameData.mainLightDirection);
    float3 light = gFrameData.mainLightColor * gFrameData.mainLightVolumetricScatteringIntensity
                 * CalculateFogPhase(dot(viewDirection, toSun)) * selfShadow
                 + gCompositeSettings.fogAmbientLight;

    return color * transmittance + gCompositeSettings.fogAlbedo * light * (1.0f - transmittance);
}

float4 main(VSOutput input) : SV_TARGET
{
    float3 sceneColor = gSceneTexture.Sample(gSampler, input.uv).rgb;
    float3 combinedScene = sceneColor;

    // DoF の合成
    if (gCompositeSettings.enableDoF != 0)
    {
        float4 dofColor = gDoFTexture.Sample(gSampler, input.uv);
        combinedScene = lerp(combinedScene, dofColor.rgb, dofColor.a * gCompositeSettings.dofStrength);
    }

    // SSAO の適用
    if (gCompositeSettings.enableSSAO != 0)
    {
        float ssao = gSSAOTexture.Sample(gSampler, input.uv).r;
        combinedScene *= ssao;
    }

    // ブルーム成分の加算
    float3 bloomColor = gBloomTexture.Sample(gSampler, input.uv).rgb;
    float3 result = combinedScene + (bloomColor * gCompositeSettings.bloomIntensity);

    // 歩ける範囲の外の、遠くの霧
    if (gCompositeSettings.boundaryEnabled != 0)
    {
        // 物と空の境目で深度が混ざらないよう、補間せずに読む
        uint depthWidth, depthHeight;
        gDepthTexture.GetDimensions(depthWidth, depthHeight);
        int2 depthCoord = min(int2(input.uv * float2(depthWidth, depthHeight)), int2(depthWidth, depthHeight) - 1);
        result = ApplyBoundaryFog(result, input.uv, gDepthTexture.Load(int3(depthCoord, 0)));
    }

    // ボリュメトリックフォグの合成
    if (gCompositeSettings.enableVolumetricFog != 0)
    {
        float4 vFogData = gVolumetricFogTexture.Sample(gSampler, input.uv);
        float3 vFogIllumination = vFogData.rgb;
        float vFogTransmittance = vFogData.a;

        result = result * vFogTransmittance + vFogIllumination;
    }

    // NaN のフォールバック処理
    if (any(isnan(result)))
    {
        result = float3(0.0f, 0.0f, 0.0f);
    }

    // トーンマッピング適用
    result = ACESFilm(clamp(result, 0.0f, 65504.0f));

    return float4(result, 1.0f);
}