#ifndef LIGHTING_UTILS_HLSLI
#define LIGHTING_UTILS_HLSLI

#include "ShaderConstants.hlsli"
#include "PBRUtils.hlsli"

// Directional Light
float3 ApplyDirectionalLights(
    SurfaceData surface, float3 toEye, float shadowFactor, const DirectionalLight dirLights[MAX_DIRECTIONAL_LIGHTS],
    Texture2D<float4> toonRamp, SamplerState clampSampler)
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    float roughness = clamp(surface.roughness, 0.05f, 1.0f);
    float metalness = saturate(surface.metalness);

    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i)
    {
        if (dirLights[i].enable == 0)
            continue;

        float3 lightDir = normalize(-dirLights[i].direction);
        float3 lightColor = dirLights[i].color.rgb * dirLights[i].color.a;
        float lightIntensity = dirLights[i].intensity;

        float NdotL = dot(surface.normal, lightDir);
        float saturateNdotL = saturate(NdotL);

        // 自己陰(NdotL)と落ち影(shadowFactor)を合わせた明るさ
        float combinedShadow = saturateNdotL;
        if (i == 0)
            combinedShadow *= shadowFactor;

        float3 radiance = float3(0.0f, 0.0f, 0.0f);

        if (surface.lightMode == SHADING_MODEL_PBR)
        {
            radiance = CalculatePBR(surface.pbrAlbedo, surface.normal, toEye, lightDir, lightColor, lightIntensity, roughness, metalness);
            if (i == 0)
                radiance *= shadowFactor; // 影の濃さが適用済みの数値をそのまま掛ける
        }
        else
        {
            float3 diffuse = float3(0.0f, 0.0f, 0.0f);
            float3 specular = float3(0.0f, 0.0f, 0.0f);

            if (surface.lightMode == SHADING_MODEL_HALFLAMBERT)
            {
                float halfLambert = pow(saturateNdotL * 0.5f + 0.5f, surface.diffuseReflection);
                if (i == 0)
                    halfLambert *= shadowFactor;
                diffuse = surface.albedo * lightColor * halfLambert * lightIntensity;
            }
            else if (surface.lightMode == SHADING_MODEL_PHONG)
            {
                diffuse = surface.albedo * lightColor * combinedShadow * lightIntensity;
                if (NdotL > 0.0f)
                {
                    float3 halfVec = normalize(lightDir + toEye);
                    float spec = pow(saturate(dot(surface.normal, halfVec)), surface.shininess);
                    specular = surface.specularColor * lightColor * spec * lightIntensity;
                    if (i == 0)
                        specular *= shadowFactor;
                }
            }
            else if (surface.lightMode == SHADING_MODEL_TOON)
            {
                float rampU = NdotL * 0.5f + 0.5f;
                if (i == 0)
                    rampU *= shadowFactor;
                float3 rampColor = toonRamp.Sample(clampSampler, float2(rampU, 0.5f)).rgb;
                diffuse = surface.albedo * rampColor * lightColor * lightIntensity;
            }
            radiance = diffuse + specular;
        }
        finalColor += radiance;
    }

    return finalColor;
}

// Point Light
float3 ApplyPointLights(
    SurfaceData surface, float3 worldPos, float3 toEye,
    const PointLight pointLights[MAX_POINT_LIGHTS])
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        if (pointLights[i].enable == 0)
            continue;
        
        float3 lightVec = pointLights[i].position - worldPos;
        float distance = length(lightVec);
        float radius = pointLights[i].radius;
        if (distance > radius)
            continue;

        float3 lightDir = (distance > 0.001f) ? (lightVec / distance) : float3(0.0f, 1.0f, 0.0f);
        float attenuation = pow(saturate(1.0f - distance / radius), 2.0f);

        float3 lightColor = pointLights[i].color.rgb;
        float3 radiance = float3(0.0f, 0.0f, 0.0f);

        if (surface.lightMode == SHADING_MODEL_PBR)
        {
            radiance = CalculatePBR(surface.pbrAlbedo, surface.normal, toEye, lightDir, lightColor, pointLights[i].intensity, surface.roughness, surface.metalness) * attenuation;
        }
        else
        {
            float ndotl = saturate(dot(surface.normal, lightDir));
            float3 diffuse = surface.albedo * lightColor * ndotl * pointLights[i].intensity * attenuation;
            
            float3 specular = float3(0, 0, 0);
            if (ndotl > 0.0f)
            {
                float3 halfVec = normalize(lightDir + toEye);
                float spec = pow(saturate(dot(surface.normal, halfVec)), surface.shininess);
                specular = surface.specularColor * lightColor * pointLights[i].intensity * spec * attenuation;
            }
            radiance = diffuse + specular;
        }
        finalColor += radiance;
    }
    return finalColor;
}

// Spot Light
float3 ApplySpotLights(
    SurfaceData surface, float3 worldPos, float3 toEye,
    const SpotLight spotLights[MAX_SPOT_LIGHTS])
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
    {
        if (spotLights[i].enable == 0)
            continue;
        
        float3 lightVecFromLight = worldPos - spotLights[i].position;
        float distance = length(lightVecFromLight);
        if (distance > spotLights[i].distance)
            continue;

        float3 dirFromLight = (distance > 0.001f) ? (lightVecFromLight / distance) : normalize(spotLights[i].direction);
        
        float distanceAtt = pow(saturate(1.0f - distance / spotLights[i].distance), 2.0f);
        float coneDot = dot(normalize(spotLights[i].direction), dirFromLight);
        float angleAtt = smoothstep(spotLights[i].cosAngle, lerp(1.0f, spotLights[i].cosAngle, 0.8f), coneDot);
        float attenuation = distanceAtt * angleAtt;
        
        if (attenuation <= 0.0f)
            continue;

        float3 lightColor = spotLights[i].color.rgb;
        float lightIntensity = spotLights[i].intensity;
        float3 lightDirL = -dirFromLight;
        float3 radiance = float3(0.0f, 0.0f, 0.0f);

        if (surface.lightMode == SHADING_MODEL_PBR)
        {
            float3 R = reflect(-toEye, surface.normal);
            float fakeSourceRadius = 0.1f;
            float3 closestPoint = lightDirL + R * clamp(dot(lightDirL, R), 0.0f, fakeSourceRadius);
            
            float alpha = surface.roughness * surface.roughness;
            float modifiedRoughness = sqrt(saturate(alpha + (fakeSourceRadius / max(distance, 0.001f) * 0.5f)));

            radiance = CalculatePBR(surface.pbrAlbedo, surface.normal, toEye, normalize(closestPoint), lightColor, lightIntensity, modifiedRoughness, surface.metalness) * attenuation;
        }
        else
        {
            float ndotl = saturate(dot(surface.normal, lightDirL));
            float3 diffuse = surface.albedo * lightColor * ndotl * lightIntensity * attenuation;
            
            float3 specular = float3(0, 0, 0);
            if (ndotl > 0.0f)
            {
                float3 halfVec = normalize(lightDirL + toEye);
                float spec = pow(saturate(dot(surface.normal, halfVec)), surface.shininess);
                specular = surface.specularColor * lightColor * lightIntensity * spec * attenuation;
            }
            radiance = diffuse + specular;
        }
        finalColor += radiance;
    }
    return finalColor;
}

// Area Light
float3 ApplyAreaLights(
    SurfaceData surface, float3 worldPos, float3 toEye,
    const AreaLight areaLights[MAX_AREA_LIGHTS])
{
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < MAX_AREA_LIGHTS; ++i)
    {
        if (areaLights[i].enable == 0)
            continue;

        float3 vecToPixel = worldPos - areaLights[i].position;
        float3 rightDir = normalize(areaLights[i].right);
        float3 upDir = normalize(areaLights[i].up);
        float halfWidth = length(areaLights[i].right);
        float halfHeight = length(areaLights[i].up);

        float projRight = clamp(dot(vecToPixel, rightDir), -halfWidth, halfWidth);
        float projUp = clamp(dot(vecToPixel, upDir), -halfHeight, halfHeight);

        float3 closestPointOnLight = areaLights[i].position + rightDir * projRight + upDir * projUp;
        float3 lightVec = closestPointOnLight - worldPos;
        float distance = length(lightVec);
        float3 lightDir = normalize(lightVec);

        float attenuation = areaLights[i].range > 0.001f
            ? pow(saturate(1.0f - distance / areaLights[i].range), areaLights[i].decay)
            : 1.0f;

        float3 lightColor = areaLights[i].color.rgb;
        float lightIntensity = areaLights[i].intensity;
        float3 radiance = float3(0.0f, 0.0f, 0.0f);

        if (surface.lightMode == SHADING_MODEL_PBR)
        {
            radiance = CalculatePBR(surface.pbrAlbedo, surface.normal, toEye, lightDir, lightColor, lightIntensity, surface.roughness, surface.metalness) * attenuation;
        }
        else
        {
            float ndotl = saturate(dot(surface.normal, lightDir));
            float3 diffuse = surface.albedo * lightColor * ndotl * lightIntensity * attenuation;
            
            float3 specular = float3(0, 0, 0);
            if (ndotl > 0.0f)
            {
                float3 halfVec = normalize(lightDir + toEye);
                float spec = pow(saturate(dot(surface.normal, halfVec)), surface.shininess);
                specular = surface.specularColor * lightColor * lightIntensity * spec * attenuation;
            }
            radiance = diffuse + specular;
        }
        finalColor += radiance;
    }
    return finalColor;
}

float3 ApplyRimLight(
    float3 normal, float3 toEye, float3 toLight,
    float rimPower, int rimUseLightDir, float3 rimColor, float rimIntensity)
{
    // 基本のリムライト
    float NdotV = saturate(dot(normal, toEye));
    float rim = 1.0f - NdotV;
    rim = pow(rim, max(rimPower, 0.001f));

    // ライト方向によるマスク処理
    if (rimUseLightDir != 0)
    {
         // ライトが当たっている面 (NdotL) の強さを掛ける
        float NdotL = saturate(dot(normal, toLight));
        rim *= NdotL;
    }

    return rimColor * rim * rimIntensity;
}

#endif