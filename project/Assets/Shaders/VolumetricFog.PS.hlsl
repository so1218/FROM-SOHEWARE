//#include "FullScreenQuad.hlsli"
//#include "ShaderConstants.hlsli"

//Texture2D<float> gDepthTexture : register(t0); // シーンの深度
//Texture2D<float> gShadowMap : register(t1); // シャドウマップ

//SamplerState gSampler : register(s0);
//SamplerComparisonState gShadowSampler : register(s1); // 影判定用

//// 定数バッファ
//ConstantBuffer<FrameData> gFrameData : register(b0);
//ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

//// Henyey-Greenstein 位相関数 (光の散乱)
//float PhaseFunctionHG(float cosTheta, float g)
//{
//    float g2 = g * g;
//    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
//    return (1.0f - g2) / (4.0f * 3.14159265f * pow(denom, 1.5f));
//}

//float DualPhaseHG(float cosTheta, float g)
//{
//    // 前方散乱（太陽方向の強い光）
//    float forward = PhaseFunctionHG(cosTheta, g);
//    // 後方散乱（光源の反対側を向いたときに見える、わずかな反射）
//    float backward = PhaseFunctionHG(cosTheta, -0.2f); // -0.2固定程度が自然です
    
//    // 9:1 くらいの割合で合成する
//    return lerp(backward, forward, 0.9f);
//}

//// 簡易的なプロシージャル3D雲ノイズ (テスト用)
//float SimpleCloudNoise(float3 p)
//{
//    // 複数の波を合成してモクモク感を作る
//    float n = sin(p.x) * sin(p.y) * sin(p.z);
//    n += sin(p.x * 2.2f + 1.1f) * sin(p.y * 2.3f + 2.2f) * sin(p.z * 2.4f + 3.3f) * 0.5f;
//    return saturate(n * 0.5f + 0.5f); // 0.0 ～ 1.0の範囲に収める
//}

//float4 main(VSOutput input) : SV_TARGET
//{
//    float depthVal = gDepthTexture.Sample(gSampler, input.uv);

//    // ワールド座標を復元
//    float clipX = input.uv.x * 2.0f - 1.0f;
//    float clipY = (1.0f - input.uv.y) * 2.0f - 1.0f;
//    float4 clipPos = float4(clipX, clipY, depthVal, 1.0f);
//    float4 worldPos = mul(clipPos, gFrameData.invViewProj);
//    worldPos /= worldPos.w;

//    // レイマーチングの準備
//    float3 rayVec = worldPos.xyz - gFrameData.cameraWorldPosition;
//    float rayLength = length(rayVec);
//    float3 rayDir = rayVec / max(rayLength, 0.0001f);

//    // 最大距離とステップ数をパラメータから取得
//    float marchLength = min(rayLength, gFogSettings.maxDistance);
//    int steps = gFogSettings.steps;
//    float stepSize = marchLength / max((float) steps, 1.0f); // 0割り防止
    
//    // ディザリング
//    float dither = frac(sin(dot(input.uv, float2(12.9898, 78.233))) * 43758.5453) * stepSize;
//    float3 currentPos = gFrameData.cameraWorldPosition + (rayDir * dither);

//    float3 volumetricIllumination = float3(0, 0, 0);
//    // 光の方向ベクトルを反転させて太陽の方向に向ける
//    float3 lightDir = normalize(-gFrameData.mainLightDirection);
//    float cosTheta = dot(rayDir, lightDir);

//    // 散乱係数をパラメータから取得
//    float phase = DualPhaseHG(cosTheta, gFogSettings.scatteringG);
    
//    // ループ開始前の準備
//    float transmittance = 1.0f; // 初期状態では光は100%透過
//    float3 ambientLight = float3(0.05f, 0.05f, 0.07f); // 暗い影の中を照らす環境光

//    // レイマーチング・ループ
//    for (int i = 0; i < steps; ++i)
//    {
//    // (1. シャドウ判定は既存の通り)
//        float4 shadowCoord = mul(float4(currentPos, 1.0f), gFrameData.lightViewProj);
//        shadowCoord.xyz /= shadowCoord.w;
//        float2 shadowUV = shadowCoord.xy * float2(0.5f, -0.5f) + 0.5f;
//        float shadowVisibility = 1.0f;
//        if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f && shadowUV.y >= 0.0f && shadowUV.y <= 1.0f && shadowCoord.z >= 0.0f && shadowCoord.z <= 1.0f)
//        {
//            float compareDepth = shadowCoord.z - 0.001f;
//            shadowVisibility = gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV, compareDepth);
//        }

//    // (2. 高さ・3. ノイズ・4. 濃度の計算は既存の通り)
//        float heightFalloff = exp(-max(currentPos.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);
//        float noiseVal = SimpleCloudNoise(currentPos * gFogSettings.noiseScale + (float3(gFrameData.gTime * gFogSettings.windSpeed, 0, 0)));
//        noiseVal = smoothstep(gFogSettings.noiseThreshold, 1.0f, noiseVal);
//        float stepDensity = gFogSettings.density * heightFalloff * noiseVal;

//    // -----------------------------------------------------
//    // ★ ここからが超重要：色の計算
//    // -----------------------------------------------------

//    // このステップでの減衰率
//        float stepAttenuation = exp(-stepDensity * stepSize);

//    // a. 太陽からの直接光 (shadowVisibilityが効く)
//        float3 directLight = shadowVisibility * phase * gFrameData.mainLightColor.rgb;

//    // b. 環境光 (アンビエント) 
//    // 固定値ではなく、UIから渡された fogColor と ambientFactor を使う
//    // さらに、メインライトが当たっていない場所(影)の環境光を少し弱めることで立体感を出す
//        float ambientOcclusion = lerp(0.4f, 1.0f, shadowVisibility);
//        float3 ambientColor = gFogSettings.fogColor * gFogSettings.ambientFactor * ambientOcclusion;

//    // c. 最終的な散乱光
//    // fogColorを全体に掛けることで、ピンクにしたら全体がピンクのトーンになる
//        float3 scatteringLight = (directLight * gFogSettings.fogColor + ambientColor);
    
//    // -----------------------------------------------------

//    // 区間内での散乱エネルギーの積分
//        float3 stepScattering = scatteringLight * (1.0f - stepAttenuation);

//    // 現在の透過率を掛け合わせて加算
//        volumetricIllumination += stepScattering * transmittance;

//    // 透過率を更新
//        transmittance *= stepAttenuation;

//        currentPos += rayDir * stepSize;
//    }
    
//    volumetricIllumination *= gFogSettings.intensity;

//    return float4(volumetricIllumination, transmittance);
//}