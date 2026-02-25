#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0); // 元のシーン
Texture2D gBloomTexture : register(t1); // Bloom用 (光のみボケ)
Texture2D gDoFTexture : register(t2); // DoF用 (全体ボケ)
Texture2D<float> gDepthTexture : register(t3); // 深度マップ
Texture2D gGodRayTexture : register(t4);
Texture2D gSSAOTexture : register(t5); // SSAOマップ
Texture2D gSSRTexture : register(t6); // SSRマップ
Texture2D gNoiseTexture : register(t7); // Noise(フォグの揺らぎ用)

SamplerState gSampler : register(s0);
SamplerState gWrapSampler : register(s1);

ConstantBuffer<CombineSettings> gCombineSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

// 深度リニア化関数
float LinearizeDepth(float d)
{
    float n = gFrameData.nearClip;
    float f = gFrameData.farClip;

    return (n * f) / (f - d * (f - n));
}
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// Tent Filter (3x3近傍サンプリングで滑らかに拡大)
float3 UpsampleTent(Texture2D tex, SamplerState s, float2 uv, float2 texelSize, float sampleScale)
{
    // サンプリングオフセット
    float4 d = texelSize.xyxy * float4(1.0, 1.0, -1.0, 0.0) * sampleScale;

    // 3x3近傍サンプリング
    float3 s1 = tex.Sample(s, uv - d.xy).rgb;
    float3 s2 = tex.Sample(s, uv - d.wy).rgb;
    float3 s3 = tex.Sample(s, uv - d.zy).rgb;
    float3 s4 = tex.Sample(s, uv - d.xw).rgb;
    float3 s5 = tex.Sample(s, uv).rgb;
    float3 s6 = tex.Sample(s, uv + d.xw).rgb;
    float3 s7 = tex.Sample(s, uv + d.zy).rgb;
    float3 s8 = tex.Sample(s, uv + d.wy).rgb;
    float3 s9 = tex.Sample(s, uv + d.xy).rgb;

    // Tent重みで合成
    return (s1 + s3 + s7 + s9) * 0.0625 +
           (s2 + s4 + s6 + s8) * 0.125 +
           s5 * 0.25;
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

float GetFogNoise(float3 worldPos, float time)
{
    // パラメータに基づいたスケーリング
    float2 uv = worldPos.xz * gCombineSettings.fogNoiseScale;
    float moveTime = time * gCombineSettings.fogNoiseSpeed;
    
    // 2枚のサンプリング（速度を変えて干渉させる）
    float2 scroll1 = float2(moveTime * 1.0, moveTime * 0.4);
    float2 scroll2 = float2(moveTime * -0.6, moveTime * 0.8);
    
    float n1 = gNoiseTexture.Sample(gWrapSampler, uv + scroll1).r;
    float n2 = gNoiseTexture.Sample(gWrapSampler, uv + scroll2).r;
    
    // 合成
    float combinedNoise = n1 * n2;

    // コントラスト調整 (Blightboundのようなパキッとした霧にするため)
    // 0.5を中心に、Contrast倍してsaturateする
    combinedNoise = saturate((combinedNoise - 0.5) * gCombineSettings.fogNoiseContrast + 0.5);
    
    return combinedNoise;
}

float4 main(VSOutput input) : SV_TARGET
{
    // 各入力テクスチャをサンプリング
    float4 sceneColor = gSceneTexture.Sample(gSampler, input.uv);
    float4 dofColor = gDoFTexture.Sample(gSampler, input.uv);
    float depthVal = gDepthTexture.Sample(gSampler, input.uv);

    // BloomテクスチャをTentフィルタでアップサンプル
    uint width, height;
    gBloomTexture.GetDimensions(width, height);
    float2 bloomTexelSize =
        float2(1.0f / float(width), 1.0f / float(height));

    float3 bloomColor =
        UpsampleTent(gBloomTexture, gSampler, input.uv, bloomTexelSize, 1.0f);
    
    // GodRayサンプリング
    float3 godRayColor = gGodRayTexture.Sample(gSampler, input.uv).rgb;

    // 深度をリニア化
    float linearDepth = LinearizeDepth(depthVal);
    
    // UVをクリップ空間に変換
    float clipX = input.uv.x * 2.0f - 1.0f;
    float clipY = (1.0f - input.uv.y) * 2.0f - 1.0f;

    // クリップ空間の座標を作成（ZにDepthを入れる）
    float4 clipPos = float4(clipX, clipY, depthVal, 1.0f);

    // 逆行列を掛けてワールド空間へ
    float4 worldPos = mul(clipPos, gFrameData.invViewProj);
    worldPos /= worldPos.w; // W除算で座標を確定

    // DoF未適用時はシーンカラーをそのまま使用
    float3 combinedScene = sceneColor.rgb;

    // 被写界深度の適用
    if (gCombineSettings.enableDoF != 0)
    {
        // ピントが合っている場所はdofColor.aが0で、SceneColorが使われる
        combinedScene = lerp(sceneColor.rgb, dofColor.rgb, dofColor.a);
    }
    
    // SSAOの適用
    if (gCombineSettings.enableSSAO != 0)
    {
        float ssao = gSSAOTexture.Sample(gSampler, input.uv).r;
        
        // ベースのシーンカラーに対してのみ影を落とす
        combinedScene *= ssao;
    }
    
    // SSRの適用
    if (gCombineSettings.enableSSR != 0)
    {
        float4 ssrColor = gSSRTexture.Sample(gSampler, input.uv);
        
        // シーンカラーに加算
        combinedScene += ssrColor.rgb * ssrColor.a * gCombineSettings.ssrIntensity;
    }

    // BloomとGodRayの加算
    float3 result = combinedScene +
                    (bloomColor * gCombineSettings.bloomIntensity) +
                    (godRayColor * gCombineSettings.godRayIntensity);

    // フォグの適用
    if (gCombineSettings.enableFog != 0)
    {
        float3 rayVec = worldPos.xyz - gFrameData.cameraWorldPosition;
        float rayLength = length(rayVec);
    
        // ゼロ除算の防止
        float3 rayDir = rayVec / max(rayLength, 0.0001f);

        // ノイズによる密度の変化
        float noise = GetFogNoise(worldPos.xyz, gFrameData.gTime);
        float animatedDensity = gCombineSettings.heightFogDensity * (0.5f + noise * 0.5f);

        float heightDiff = rayVec.y;
        // 微小値の扱いをより安全に
        if (abs(heightDiff) < 0.001f)
            heightDiff = (heightDiff >= 0) ? 0.001f : -0.001f;

        float camHeight = gFrameData.cameraWorldPosition.y - gCombineSettings.heightFogBaseHeight;
        float pixHeight = worldPos.y - gCombineSettings.heightFogBaseHeight;
    
        // falloffが0の場合のクラッシュ防止
        float falloff = max(gCombineSettings.heightFogFalloff, 0.0001f);

        // ハイトフォグの公式
        float fogAmount = (exp(-falloff * camHeight) - exp(-falloff * pixHeight)) / (falloff * heightDiff);
        float heightFogFactor = saturate(1.0f - exp(-animatedDensity * fogAmount * rayLength));

        // 太陽光による散乱 
        float3 lightDir = normalize(gFrameData.mainLightDirection);
        float scattering = pow(saturate(dot(rayDir, lightDir)), 4.0f);
    
        // 太陽の色を少し強めにしてフォグに乗せる
        float3 scatteringColor = gFrameData.mainLightColor.rgb * 2.0f;
        float3 fogColor = lerp(gCombineSettings.fogColor.rgb, scatteringColor, scattering);

        // 距離フォグ
        float distFogFactor = saturate((linearDepth - gCombineSettings.distanceFogStart) /
                                   max(gCombineSettings.distanceFogEnd - gCombineSettings.distanceFogStart, 0.0001f));
        distFogFactor = smoothstep(0.0, 1.0, distFogFactor);

        float finalFogFactor = max(heightFogFactor, distFogFactor);

        // 最終合成
        result = lerp(result, fogColor, finalFogFactor);
    }

    // NaN対策
    if (any(isnan(result)))
    {
        result = float3(0.0, 0.0, 0.0);
    }

    // トーンマッピング前のクランプ
    result = clamp(result, 0.0, 65504.0);

    // トーンマッピング
    result = ACESFilm(result);

    return float4(result, 1.0f);
}