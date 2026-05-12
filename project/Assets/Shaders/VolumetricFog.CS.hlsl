#include "ShaderConstants.hlsli"

// --- 入力リソース ---
Texture2D<float> gDepthTexture : register(t0);
Texture2D<float> gShadowMap : register(t1);
Texture3D<float> gNoiseVolume : register(t2);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

// --- 出力リソース (UAV: Unordered Access View) ---
// ★追加: 書き込み可能な2Dテクスチャ
RWTexture2D<float4> gOutput : register(u0);

// --- 定数バッファ ---
ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

// Henyey-Greenstein 位相関数 (光の散乱)
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
    float backward = PhaseFunctionHG(cosTheta, -0.2f); // -0.2固定程度が自然です
    
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

// ==========================================================
// コンピュートシェーダーのエントリーポイント
// ==========================================================
// 8x8のピクセルブロック（スレッドグループ）ごとに並列処理する
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // 1. 画面外の処理を弾く（テクスチャサイズより外側のスレッドが走った場合）
    uint width, height;
    gOutput.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    // 2. UV座標の計算
    // スレッドID(ピクセル座標)の中心(+0.5)を取り、解像度で割って 0.0～1.0 にする
    float2 uv = (float2(DTid.xy) + 0.5f) / float2(width, height);

    // 3. 深度のサンプリング
    // ★プロのテクニック: CSで1対1のピクセルを読むなら、SampleよりLoadが正確で高速です。
    // Loadには int3(x, y, mipLevel) を渡します。
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

   // ★修正：1歩の長さを「最大距離 ÷ 最大ステップ数」で完全に固定する
    float stepSize = gFogSettings.maxDistance / max((float) gFogSettings.steps, 1.0f);
    
    // ★修正：このピクセルが「何歩進んだら物体にぶつかるか」を計算する
    float marchLength = min(rayLength, gFogSettings.maxDistance);
    int actualSteps = min(gFogSettings.steps, (int) ceil(marchLength / stepSize));
    
    // ディザリング
   // ディザリング（BayerマトリクスやInterleaved Gradient Noiseなどを使うと綺麗です）
// 今回は少し質の良い擬似乱数(IGN)に変更
    float dither = frac(52.9829189f * frac(dot(DTid.xy, float2(0.06711056f, 0.00583715f))));

// ★超重要：レイの開始位置を、ステップサイズの範囲でランダムにズラす
// これにより、隣のピクセルと「層」の位置がズレて円形が消えます
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

        // (2. 高さ・3. ノイズ・4. 濃度の計算は既存の通り)
        float heightFalloff = exp(-max(currentPos.y - gFogSettings.baseHeight, 0.0f) * gFogSettings.heightFalloff);

// -----------------------------------------------------
        // ★ 修正：エロージョン（浸食）によるモクモク感の強調
        // -----------------------------------------------------
        // 1層目（ベースとなる大きな霧の塊）
        float3 uvw1 = currentPos * gFogSettings.noiseScale;
        uvw1 += float3(gFrameData.gTime * 0.05f, 0.0f, gFrameData.gTime * 0.02f);
        float noise1 = gNoiseVolume.SampleLevel(gSampler, frac(uvw1), 0).r;

        // 2層目（輪郭を削るための細かなディテール）
        float3 uvw2 = currentPos * (gFogSettings.noiseScale * 3.0f); // スケールを少し大きめ(細かく)する
        uvw2 += float3(-gFrameData.gTime * 0.08f, gFrameData.gTime * 0.03f, 0.0f);
        float noise2 = gNoiseVolume.SampleLevel(gSampler, frac(uvw2), 0).r;

        // ★変更点1：足し算ではなく、ベースの塊からディテールを「引いて削る」
        // これにより、カリフラワーのような凹凸のある輪郭が生まれます
        float combinedNoise = saturate(noise1 - (1.0f - noise2) * 0.3f);

        // ★変更点2：境界を「鋭く」切り落とす
        // smoothstep(閾値, 1.0f, x) だとグラデーションが広すぎるので、
        // 上限を (閾値 + 0.1f) くらいに狭めることで、エッジの効いた塊になります
        float edgeSoftness = 0.15f; // 値が小さいほど輪郭がクッキリする（0.05〜0.2くらいがおすすめ）
        float noiseVal = smoothstep(gFogSettings.noiseThreshold, gFogSettings.noiseThreshold + edgeSoftness, combinedNoise);

        // ★変更点3：塊の中身を「強烈に濃く」する
        // モクモク感を出すには、少し進んだだけで光が遮断されるほどの密度が必要です
        // noiseVal が 0.0 より大きい部分（霧が存在する部分）の密度を跳ね上げます
        float densityMultiplier = 5.0f; // ★ ここを 2.0 ～ 10.0 などで調整してみてください
        float stepDensity = gFogSettings.density * densityMultiplier * heightFalloff * noiseVal;

        // -----------------------------------------------------
        // ★ ここからが超重要：色の計算
        // -----------------------------------------------------

        // このステップでの減衰率
        float stepAttenuation = exp(-stepDensity * stepSize);

        // a. 太陽からの直接光 (shadowVisibilityが効く)
        float3 directLight = shadowVisibility * phase * gFrameData.mainLightColor.rgb;

        // b. 環境光 (アンビエント) 
        // 固定値ではなく、UIから渡された fogColor と ambientFactor を使う
        // さらに、メインライトが当たっていない場所(影)の環境光を少し弱めることで立体感を出す
        float ambientOcclusion = lerp(0.4f, 1.0f, shadowVisibility);
        float3 ambientColor = gFogSettings.fogColor * gFogSettings.ambientFactor * ambientOcclusion;

        // c. 最終的な散乱光
        // fogColorを全体に掛けることで、ピンクにしたら全体がピンクのトーンになる
        float3 scatteringLight = (directLight * gFogSettings.fogColor + ambientColor);
    
        // -----------------------------------------------------

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

    // ★最後: リターンするのではなく、テクスチャの特定座標に書き込む
    gOutput[DTid.xy] = float4(volumetricIllumination, transmittance);
}