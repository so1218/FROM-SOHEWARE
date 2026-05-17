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

// 簡易的なプロシージャル3D雲ノイズ (テスト用)
float SimpleCloudNoise(float3 p)
{
    // 複数の波を合成してモクモク感を作る
    float n = sin(p.x) * sin(p.y) * sin(p.z);
    n += sin(p.x * 2.2f + 1.1f) * sin(p.y * 2.3f + 2.2f) * sin(p.z * 2.4f + 3.3f) * 0.5f;
    return saturate(n * 0.5f + 0.5f); // 0.0 ～ 1.0の範囲に収める
}

// コンピュートシェーダーのエントリーポイント
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // 画面外の処理を弾く
    uint width, height;
    gOutput.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    // UV座標の計算
    float2 uv = (float2(DTid.xy) + 0.5f) / float2(width, height);

    // 深度のサンプリング
    float depthVal = gDepthTexture.SampleLevel(gSampler, uv, 0).r;

    // ワールド座標を復元
    float clipX = uv.x * 2.0f - 1.0f;
    float clipY = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(clipX, clipY, depthVal, 1.0f);
    float4 worldPos = mul(clipPos, gFrameData.invViewProj);
    worldPos /= worldPos.w;

    // レイマーチングの準備
    float3 rayVec = worldPos.xyz - gFrameData.cameraWorldPosition;
    float rayLength = length(rayVec);
    float3 rayDir = rayVec / max(rayLength, 0.0001f);

   // 1歩の長さを最大距離 ÷ 最大ステップ数で完全に固定
    float stepSize = gFogSettings.maxDistance / max((float) gFogSettings.steps, 1.0f);
    
    // このピクセルが何歩進んだら物体にぶつかるかを計算
    float marchLength = min(rayLength, gFogSettings.maxDistance);
    int actualSteps = min(gFogSettings.steps, (int) ceil(marchLength / stepSize));
    
    // ディザリング
    float dither = frac(52.9829189f * frac(dot(DTid.xy, float2(0.06711056f, 0.00583715f))));

    // レイの開始位置を、ステップサイズの範囲でランダムにズラす
    float3 currentPos = gFrameData.cameraWorldPosition + (rayDir * (stepSize * dither));

    float3 volumetricIllumination = float3(0, 0, 0);
    // 光の方向ベクトルを反転させて太陽の方向に向ける
    float3 lightDir = normalize(-gFrameData.mainLightDirection);
    float cosTheta = dot(rayDir, lightDir);

    // 散乱係数をパラメータから取得
    float phase = DualPhaseHG(cosTheta, gFogSettings.scatteringG);
    
    // ループ開始前の準備
    float transmittance = 1.0f; // 初期状態では光は100%透過
    float3 ambientLight = float3(0.05f, 0.05f, 0.07f); // 暗い影の中を照らす環境光

    // レイマーチング・ループ
    for (int i = 0; i < actualSteps; ++i)
    {
        // (1. シャドウ判定は既存の通り)
        float4 shadowCoord = mul(float4(currentPos, 1.0f), gFrameData.lightViewProj);
        shadowCoord.xyz /= shadowCoord.w;
        float2 shadowUV = shadowCoord.xy * float2(0.5f, -0.5f) + 0.5f;
        float shadowVisibility = 1.0f;
        if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f && shadowUV.y >= 0.0f && shadowUV.y <= 1.0f && shadowCoord.z >= 0.0f && shadowCoord.z <= 1.0f)
        {
            float compareDepth = shadowCoord.z - 0.005f;
            shadowVisibility = gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV, compareDepth);
        }
        
        float heightFalloff = exp(-max(currentPos.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);
        
        float3 warpUVW = currentPos * (gFogSettings.noiseScale * 0.5f);
        warpUVW += float3(gFrameData.gTime * 0.02f, gFrameData.gTime * 0.01f, gFrameData.gTime * 0.015f);
        
        float dx = gNoiseVolume.SampleLevel(gSampler, frac(warpUVW), 0);
        float dy = gNoiseVolume.SampleLevel(gSampler, frac(warpUVW + 0.33f), 0);
        float dz = gNoiseVolume.SampleLevel(gSampler, frac(warpUVW + 0.67f), 0);
        
        float3 distortion = float3(dx, dy, dz);
        
        float3 distortedPos = currentPos + (distortion * 2.0f - 1.0f) * 0.2f;
        
        float3 uvw1 = distortedPos * gFogSettings.noiseScale;
        uvw1 += float3(gFrameData.gTime * 0.05f, 0.0f, gFrameData.gTime * 0.02f);
        float noise1 = gNoiseVolume.SampleLevel(gSampler, frac(uvw1), 0).r;
        
        float3 uvw2 = distortedPos * (gFogSettings.noiseScale * 3.0f);
        uvw2 += float3(-gFrameData.gTime * 0.08f, gFrameData.gTime * 0.03f, 0.0f);
        float noise2 = gNoiseVolume.SampleLevel(gSampler, frac(uvw2), 0).r;

        float combinedNoise = saturate(noise1 - (1.0f - noise2) * 0.3f);

        // 境界を鋭く切り落とす
        float edgeSoftness = 0.15f; 
        float noiseVal = smoothstep(gFogSettings.noiseThreshold, gFogSettings.noiseThreshold + edgeSoftness, combinedNoise);

        // 塊の中身を強烈に濃くする
        float densityMultiplier = 5.0f;
        float stepDensity = gFogSettings.density * densityMultiplier * heightFalloff * noiseVal;
        
        // 復活させる部分（太陽光と環境光のベース計算）
        float stepAttenuation = exp(-stepDensity * stepSize);

        // 太陽からの直接光 (shadowVisibilityが効く)
        float3 directLight = shadowVisibility * phase * gFrameData.mainLightColor.rgb;

        // 環境光
        float ambientOcclusion = lerp(0.4f, 1.0f, shadowVisibility);
        float3 ambientColor = gFogSettings.fogColor * gFogSettings.ambientFactor * ambientOcclusion;
        
        // ローカルライトの計算
        float3 localLightScattering = float3(0, 0, 0);
        
        // 霧専用の光量ブースト
        float volumetricScatteringMultiplier = 10.0f; // 調整検討
        
        // ポイントライトの計算
        for (int p = 0; p < MAX_POINT_LIGHTS; ++p)
        {
            if (gPointLights[p].enable == 0)
                continue;

            float3 lightVec = gPointLights[p].position - currentPos;
            float dist = length(lightVec);
            if (dist > gPointLights[p].radius)
                continue;

            float3 lDir = lightVec / dist;
            float distRatio = saturate(1.0f - (dist / gPointLights[p].radius));
            float attenuation = pow(distRatio, gPointLights[p].decay);

            // 太陽のgではなく、横からでも見えるように低いg(0.3)でDualPhaseを使う
            float cosThetaLocal = dot(rayDir, lDir);
            float phaseLocal = DualPhaseHG(cosThetaLocal, 0.3f);

            localLightScattering += gPointLights[p].color.rgb * gPointLights[p].intensity * volumetricScatteringMultiplier * attenuation * phaseLocal;
        }
        
        // スポットライトの計算
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

            // Object3D側の計算式（pow 2.0f）に合わせてコントラストを高める
            float angleFalloff = saturate((currentCos - gSpotLights[s].cosAngle) / (1.0f - gSpotLights[s].cosAngle));
            angleFalloff = pow(angleFalloff, 2.0f);
            
            float distRatio = saturate(1.0f - (dist / gSpotLights[s].distance));
            float distFalloff = pow(distRatio, gSpotLights[s].decay);
            float attenuation = angleFalloff * distFalloff;

            // こちらも低いgで横からの視認性を確保
            float cosThetaLocal = dot(rayDir, lDir);
            float phaseLocal = DualPhaseHG(cosThetaLocal, 0.3f);

            // 将来的には、ここに Spotlight 用のシャドウ判定 (shadowVisibilitySpot) を掛ける
            localLightScattering += gSpotLights[s].color.rgb * gSpotLights[s].intensity * volumetricScatteringMultiplier * attenuation * phaseLocal;
        }

        // 最終的な散乱光（太陽 ＋ 環境光 ＋ ローカルライト）
        float3 scatteringLight = (directLight * gFogSettings.fogColor) + ambientColor + localLightScattering;

        // 区間内での散乱エネルギーの積分
        float3 stepScattering = scatteringLight * (1.0f - stepAttenuation);

        // 現在の透過率を掛け合わせて加算
        volumetricIllumination += stepScattering * transmittance;

        // 透過率を更新
        transmittance *= stepAttenuation;

        currentPos += rayDir * stepSize;
    }
    
    // (ループ後の最終結果)
    volumetricIllumination *= gFogSettings.intensity;

    // リターンするのではなく、テクスチャの特定座標に書き込む
    gOutput[DTid.xy] = float4(volumetricIllumination, transmittance);
}