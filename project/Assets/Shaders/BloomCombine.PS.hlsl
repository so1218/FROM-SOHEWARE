#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0); // 元のシーン
Texture2D gBloomTexture : register(t1); // Bloom用 (光のみボケ)
Texture2D gDoFTexture : register(t2); // DoF用 (全体ボケ)
Texture2D<float> gDepthTexture : register(t3); // 深度マップ
Texture2D gGodRayTexture : register(t4);
Texture2D gSSAOTexture : register(t5); // SSAOマップ

SamplerState gSampler : register(s0);

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

    // BloomとGodRayの加算
    float3 result = combinedScene +
                    (bloomColor * gCombineSettings.bloomIntensity) +
                    (godRayColor * gCombineSettings.godRayIntensity);

    // フォグの適用
    if (gCombineSettings.enableFog != 0)
    {
        // ハイトフォグの計算
        float3 rayDir = worldPos.xyz - gFrameData.cameraWorldPosition;
        float rayLength = length(rayDir); // ハイトフォグはカメラ位置からすぐ発生
        
        float heightDiff = rayDir.y;
        if (abs(heightDiff) < 0.001f)
        {
            heightDiff = 0.001f;
        }

        float camHeight = gFrameData.cameraWorldPosition.y - gCombineSettings.heightFogBaseHeight;
        float pixHeight = worldPos.y - gCombineSettings.heightFogBaseHeight;
        float falloff = gCombineSettings.heightFogFalloff;

        float fogAmount = (exp(-falloff * camHeight) - exp(-falloff * pixHeight)) / (falloff * heightDiff);
        
        // ハイトフォグの最終的な濃さ (0.0 ～ 1.0)
        float heightFogFactor = exp(-gCombineSettings.heightFogDensity * fogAmount * rayLength);
        heightFogFactor = saturate(1.0f - heightFogFactor);
        
        // 距離フォグの計算
        float distFogFactor = 0.0f;
        float distStart = gCombineSettings.distanceFogStart;
        float distEnd = gCombineSettings.distanceFogEnd;
        
        // StartとEndの距離に応じて 0.0 ～ 1.0 で線形補間
        if (distEnd > distStart)
        {
            distFogFactor = saturate((linearDepth - distStart) / (distEnd - distStart));
            
            // smoothstep を使い、霧の始まりと終わりがより自然に
            distFogFactor = smoothstep(0.0f, 1.0f, distFogFactor);
        }

        // ２つの霧を合成
        float finalFogFactor = max(heightFogFactor, distFogFactor);

        result = lerp(result, gCombineSettings.fogColor, finalFogFactor);
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