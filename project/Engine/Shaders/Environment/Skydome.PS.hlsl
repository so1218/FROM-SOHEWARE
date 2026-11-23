#include "Common/ShaderConstants.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

cbuffer DirectionalLights : register(b1)
{
    DirectionalLight gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

ConstantBuffer<AtmosphereSkyData> gSky : register(b6);

Texture2D<float4> gCloudTexture : register(t0);
SamplerState gSampler : register(s0);

struct SkydomePSInput
{
    float4 position : SV_Position;
    float3 viewDirection : TEXCOORD0;
};

// 3 枚のノイズを大きさと速さを変えて流し、重ねて雲の濃さを作る。
// 細かい層ほど速く、少し違う向きに流して、同じ模様がそろって動いて見えないようにする
float SampleCloudDensity(float2 cloudUV)
{
    float time = gFrameData.time;
    float2 wind = gSky.windVelocity;

    float largeLayer = gCloudTexture.Sample(gSampler, cloudUV + wind * time).r;
    float mediumLayer = gCloudTexture.Sample(gSampler, cloudUV * 2.0f + (wind * 1.5f + float2(-0.003f, 0.009f)) * time).r;
    float smallLayer = gCloudTexture.Sample(gSampler, cloudUV * 4.0f + (wind * 2.0f + float2(0.004f, -0.002f)) * time).r;

    return largeLayer * 0.6f + mediumLayer * 0.3f + smallLayer * 0.1f;
}

// 星の色
float3 CalculateStarColor(float temperature)
{
    const float3 blueWhite = float3(0.65f, 0.75f, 1.0f);
    const float3 white = float3(1.0f, 1.0f, 1.0f);
    const float3 yellow = float3(1.0f, 0.88f, 0.7f);
    const float3 orange = float3(1.0f, 0.65f, 0.4f);
    if (temperature < 0.3f)
        return lerp(blueWhite, white, temperature / 0.3f);
    if (temperature < 0.75f)
        return lerp(white, yellow, (temperature - 0.3f) / 0.45f);
    return lerp(yellow, orange, (temperature - 0.75f) / 0.25f);
}

// 星
float3 CalculateStars(float3 direction, float horizonRatio)
{
    float3 absoluteDirection = abs(direction);
    float faceIndex;
    float2 faceUV;
    if (absoluteDirection.x >= absoluteDirection.y && absoluteDirection.x >= absoluteDirection.z)
    {
        faceIndex = (direction.x > 0.0f) ? 0.0f : 1.0f;
        faceUV = direction.yz / absoluteDirection.x;
    }
    else if (absoluteDirection.y >= absoluteDirection.z)
    {
        faceIndex = (direction.y > 0.0f) ? 2.0f : 3.0f;
        faceUV = direction.xz / absoluteDirection.y;
    }
    else
    {
        faceIndex = (direction.z > 0.0f) ? 4.0f : 5.0f;
        faceUV = direction.xy / absoluteDirection.z;
    }

    float2 gridPosition = (faceUV * 0.5f + 0.5f) * gSky.starGridSize;
    float2 cell = floor(gridPosition);
    float2 local = gridPosition - cell;

    // 1 画素がマスのどれだけにあたるか
    float pixelSize = max(fwidth(gridPosition.x), fwidth(gridPosition.y));

    float3 placementRandom = Hash33(float3(cell, faceIndex * 1000.0f + 17.0f)) * 0.5f + 0.5f;
    if (placementRandom.x > gSky.starDensity)
        return float3(0.0f, 0.0f, 0.0f);
    float3 shapeRandom = Hash33(float3(cell + 31.7f, faceIndex * 1000.0f + 53.0f)) * 0.5f + 0.5f;

    // マスの端に寄せないので、隣のマスの星がはみ出してくることはない
    float2 starPosition = 0.2f + shapeRandom.xy * 0.6f;
    float radius = max(gSky.starSize, pixelSize * 0.75f);
    float distanceToStar = length(local - starPosition);
    float shape = exp(-2.0f * distanceToStar * distanceToStar / (radius * radius));

    // 画素に合わせて広げた分だけ暗くし、見かけの明るさをそろえる
    float energy = (gSky.starSize * gSky.starSize) / (radius * radius);

    // 暗い星ほど多く、明るい星はまれにする
    float magnitude = lerp(0.05f, 1.0f, pow(placementRandom.y, 6.0f));

    // 星ごとに速さと位相をずらして、ゆっくり瞬かせる。地平線に近いほど空気の揺らぎで強く瞬く
    float phase = shapeRandom.z * 6.2831853f;
    float speed = gSky.starTwinkleSpeed * (0.7f + frac(placementRandom.y * 13.7f) * 0.6f);
    float time = gFrameData.time;
    float twinkle = 0.6f * sin(time * speed + phase) + 0.4f * sin(time * speed * 2.3f + phase * 1.7f);
    float twinkleAmount = gSky.starTwinkleAmount * lerp(1.5f, 1.0f, horizonRatio);
    float twinkleBrightness = saturate(1.0f + twinkle * twinkleAmount);

    return CalculateStarColor(placementRandom.z) * magnitude * shape * energy * twinkleBrightness * gSky.starBrightness;
}

float4 main(SkydomePSInput input) : SV_Target
{
    float3 viewDirection = normalize(input.viewDirection);
    float3 sunDirection = normalize(-gSky.sunDirection);
    float3 moonDirection = -sunDirection;

    // 雲を照らす光
    float3 lightDirection = normalize(-gDirectionalLights[0].direction);
    float3 lightColor = gDirectionalLights[0].color.rgb * gDirectionalLights[0].intensity;

    float sunDot = saturate(dot(viewDirection, sunDirection));
    float moonDot = saturate(dot(viewDirection, moonDirection));

    // 空: 地平線から天頂へのグラデーションに、太陽の周りの明るさを足す
    float3 skyColor = lerp(gSky.horizonColor, gSky.zenithColor, pow(saturate(viewDirection.y), gSky.skyGradientExponent));
    skyColor += gSky.horizonColor * pow(sunDot, 4.0f) * gSky.sunAtmosphereGlow * max(1.0f - viewDirection.y, 0.0f);

    // 雲: 空の平面に投影する。地平線付近で伸びすぎないよう、高さを少し丸めてから割る
    float viewHeight = max(viewDirection.y, 0.0f);
    float2 cloudUV = viewDirection.xz / sqrt(viewHeight * viewHeight + 0.02f) * gSky.cloudScale;

    // 濃さの傾きから、もこもこした凹凸の法線を作る
    float density = SampleCloudDensity(cloudUV);
    float densityRight = SampleCloudDensity(cloudUV + float2(0.005f, 0.0f));
    float densityForward = SampleCloudDensity(cloudUV + float2(0.0f, 0.005f));
    float3 cloudNormal = normalize(float3(density - densityRight, gSky.cloudBumpScale, density - densityForward));

    // 濃さが cloudThreshold を超えた所が雲。超えた分が多いほど厚い。地平線の近くは消す
    float cloudAlpha = smoothstep(gSky.cloudThreshold, gSky.cloudThreshold + gSky.cloudEdgeSoftness, density)
                     * smoothstep(0.05f, 0.2f, viewDirection.y);
    float thickness = saturate((density - gSky.cloudThreshold) / max(1.0f - gSky.cloudThreshold, 0.001f));

    // 光の当たる側は明るく、厚い所ほど光が通らず暗い。陰の側は空の色を受ける
    float halfLambert = saturate(dot(cloudNormal, lightDirection) * 0.5f + 0.5f);
    float transmittance = exp(-thickness * gSky.cloudAbsorption * 3.0f);
    float3 cloudColor = lerp(skyColor * gSky.cloudShadowBrightness, lightColor * transmittance, halfLambert);

    // 薄いふちは光が透けて明るく見える
    cloudColor += lightColor * pow(1.0f - thickness, 2.0f) * cloudAlpha * 0.5f;

    // 太陽: 中心の円盤と、そのすぐ外側の光。低い位置ほど赤くする
    float sunHeight = saturate(sunDirection.y);
    float3 sunsetTint = lerp(float3(1.0f, 0.3f, 0.05f), float3(1.0f, 1.0f, 1.0f), smoothstep(0.0f, 0.2f, sunHeight));
    float3 sunDiskColor = lerp(float3(1.0f, 0.8f, 0.5f), float3(1.0f, 0.99f, 0.98f), sunHeight) * 600.0f;
    float3 sunHaloColor = lerp(float3(1.0f, 0.1f, 0.0f), float3(1.0f, 0.9f, 0.7f), sunHeight) * sunsetTint * 240.0f;
    float3 sun = pow(sunDot, 10000.0f) * sunDiskColor + pow(sunDot, 5000.0f) * sunHaloColor;

    // 月: 太陽の反対側
    float moonBrightness = smoothstep(0.999f, 0.9995f, moonDot) * 2.0f + pow(moonDot, 3000.0f) * 2.0f;
    float3 moon = moonBrightness * float3(0.6f, 0.8f, 1.0f);

    // 厚い雲の後ろにある太陽と月は隠す
    float celestialVisibility = 1.0f - cloudAlpha * thickness;

    // 星は空と一緒に回るよう、太陽が回った角度だけ見る向きを戻してから探す
    float skyAngle = atan2(gSky.sunDirection.y, gSky.sunDirection.x);
    float cosAngle = cos(skyAngle);
    float sinAngle = sin(skyAngle);
    float3 starDirection = float3(viewDirection.x * cosAngle + viewDirection.y * sinAngle,
                                  -viewDirection.x * sinAngle + viewDirection.y * cosAngle,
                                  viewDirection.z);

    // 太陽が沈むほど見え、地平線の近くは空気で薄れる
    float horizonRatio = smoothstep(0.0f, 0.3f, viewDirection.y);
    float nightRatio = 1.0f - smoothstep(-0.15f, 0.05f, sunDirection.y);
    float3 stars = CalculateStars(starDirection, horizonRatio) * nightRatio * smoothstep(0.0f, 0.15f, viewDirection.y) * (1.0f - cloudAlpha);

    float3 color = lerp(skyColor, cloudColor, cloudAlpha) + (sun + moon) * celestialVisibility + stars;
    return float4(color, 1.0f);
}
