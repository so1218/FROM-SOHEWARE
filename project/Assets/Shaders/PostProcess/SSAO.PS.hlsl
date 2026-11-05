#include "Common/FullScreenQuad.hlsli"
#include "Common/ShaderConstants.hlsli"
#include "Common/CameraUtils.hlsli"
#include "Common/MathUtils.hlsli"

ConstantBuffer<SSAOSettings> gSSAOSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

Texture2D<float4> gNormalTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);

// 半球の中を何周しながら外へ広げていくか
static const float kSpiralTurns = 7.0f;
// 深度バッファは遠いほど粗くなるので、遮っているとみなす高さを距離に比例して増やす
static const float kDistanceBiasScale = 0.001f;

// 4x4 のピクセルで回転をずらす。後のぼかしで平均されて模様が消える
static const float kRotationPattern[16] =
{
     0.0f,  8.0f,  2.0f, 10.0f,
    12.0f,  4.0f, 14.0f,  6.0f,
     3.0f, 11.0f,  1.0f,  9.0f,
    15.0f,  7.0f, 13.0f,  5.0f
};

float4 main(VSOutput input) : SV_TARGET
{
    uint depthWidth, depthHeight;
    gDepthTexture.GetDimensions(depthWidth, depthHeight);
    float2 depthSize = float2(depthWidth, depthHeight);
    int2 maxCoord = int2(depthWidth, depthHeight) - 1;

    // 深度と法線は補間すると輪郭で手前と奥が混ざるので、一番近いピクセルを読む
    int2 centerCoord = min(int2(input.uv * depthSize), maxCoord);
    float depth = gDepthTexture.Load(int3(centerCoord, 0));
    if (depth >= 1.0f)
        return float4(1.0f, 1.0f, 1.0f, 1.0f);

    float2 centerUV = (float2(centerCoord) + 0.5f) / depthSize;
    float3 viewPosition = GetViewPos(centerUV, depth, gFrameData.invProjMatrix);
    float3 worldNormal = gNormalTexture.Load(int3(centerCoord, 0)).xyz;
    float3 viewNormal = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));
    float heightBias = gSSAOSettings.bias + viewPosition.z * kDistanceBiasScale;

    // 法線を Z にした半球の向き。法線が Z とほぼ同じ向きのときは cross が 0 になるので X を使う
    float3 reference = abs(viewNormal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(reference, viewNormal));
    float3 bitangent = cross(viewNormal, tangent);
    float3x3 tangentToView = float3x3(tangent, bitangent, viewNormal);

    uint2 patternCoord = uint2(input.position.xy) % 4;
    float randomAngle = (kRotationPattern[patternCoord.y * 4 + patternCoord.x] + 0.5f) / 16.0f * 2.0f * PI;

    int sampleCount = max(gSSAOSettings.sampleCount, 1);
    float occlusion = 0.0f;

    [loop]
    for (int i = 0; i < sampleCount; ++i)
    {
        float ratio = (float(i) + 0.5f) / float(sampleCount);
        float theta = ratio * 2.0f * PI * kSpiralTurns + randomAngle;
        float radius = sqrt(ratio);
        float3 hemisphereDirection = float3(radius * cos(theta), radius * sin(theta), sqrt(1.0f - ratio));

        // 中心の近くにサンプルを集める
        float3 sampleOffset = mul(hemisphereDirection, tangentToView) * lerp(0.1f, 1.0f, ratio * ratio);
        float3 samplePosition = viewPosition + sampleOffset * gSSAOSettings.radius;

        float4 sampleClip = mul(float4(samplePosition, 1.0f), gFrameData.projectionMatrix);
        float2 sampleUV = sampleClip.xy / sampleClip.w * float2(0.5f, -0.5f) + 0.5f;
        if (any(sampleUV < 0.0f) || any(sampleUV > 1.0f))
            continue;

        // サンプルが映るピクセルにある物の位置
        int2 sampleCoord = min(int2(sampleUV * depthSize), maxCoord);
        float sceneDepth = gDepthTexture.Load(int3(sampleCoord, 0));
        float3 scenePosition = GetViewPos((float2(sampleCoord) + 0.5f) / depthSize, sceneDepth, gFrameData.invProjMatrix);

        // 中心の面より上に出ている物だけが光を遮る
        float3 toScene = scenePosition - viewPosition;
        float distanceToScene = length(toScene);
        if (dot(toScene, viewNormal) > heightBias)
        {
            // 半径の 2 倍より遠い物は遮らないものとして、だんだん弱める
            occlusion += 1.0f - saturate(distanceToScene / gSSAOSettings.radius - 1.0f);
        }
    }

    float ambientVisibility = 1.0f - occlusion / float(sampleCount);

    // 遠くは消していく
    float fadeRange = max(gSSAOSettings.fadeEnd - gSSAOSettings.fadeStart, kEpsilon);
    ambientVisibility = lerp(ambientVisibility, 1.0f, saturate((viewPosition.z - gSSAOSettings.fadeStart) / fadeRange));

    // intensity が大きいほど暗くなる
    ambientVisibility = pow(ambientVisibility, gSSAOSettings.intensity);

    return float4(ambientVisibility, ambientVisibility, ambientVisibility, 1.0f);
}
