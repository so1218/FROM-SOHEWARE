#include "Common/FullScreenQuad.hlsli"
#include "Common/ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0); 
Texture2D gBloomTexture : register(t1); // Bloom用 (光のみボケ)
Texture2D gDoFTexture : register(t2); // DoF用 (全体ボケ)
Texture2D<float> gDepthTexture : register(t3);
Texture2D gVolumetricFogTexture : register(t4);
Texture2D gSSAOTexture : register(t5);
Texture2D gSSRTexture : register(t6); 

SamplerState gSampler : register(s0);
SamplerState gWrapSampler : register(s1);

ConstantBuffer<CombineSettings> gCombineSettings : register(b0);
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

// Tent Filter (3x3近傍サンプリングで拡大)
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
    float4 vFogData = gVolumetricFogTexture.Sample(gSampler, input.uv);
    
    float3 vFogIllumination = vFogData.rgb; // 霧によって散乱して届く光
    float vFogTransmittance = vFogData.a; // 霧を通り抜けてくる背景の透過率

    // 深度をリニア化
    float linearDepth = LinearizeDepth(depthVal);
    
    // UVをクリップ空間に変換
    float clipX = input.uv.x * 2.0f - 1.0f;
    float clipY = (1.0f - input.uv.y) * 2.0f - 1.0f;

    // クリップ空間の座標を作成（ZにDepth）
    float4 clipPos = float4(clipX, clipY, depthVal, 1.0f);

    // 逆行列を掛けてワールド空間へ
    float4 worldPos = mul(clipPos, gFrameData.invViewProj);
    worldPos /= worldPos.w; // W除算

    // DoF未適用時はシーンカラー
    float3 combinedScene = sceneColor.rgb;

    // 被写界深度の適用
    if (gCombineSettings.enableDoF != 0)
    {
        // ピントが合っている場所はSceneColor
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

   // Bloomの加算
    float3 bloomColor = gBloomTexture.Sample(gSampler, input.uv).rgb;
    
    // シーンの色を確定
    float3 result = combinedScene + (bloomColor * gCombineSettings.bloomIntensity);

    // Volumetric Fog の適用 (物理合成)
    if (gCombineSettings.enableVolumetricFog != 0)
    {
        // 背景（result）を透過率で暗くし、霧の光を加算する
        result = result * vFogTransmittance + vFogIllumination;
    }
    
    // NaN対策
    if (any(isnan(result)))
    {
        result = float3(0.0, 0.0, 0.0);
    }

   // 最終出力処理 (トーンマップ等)
    result = ACESFilm(clamp(result, 0.0, 65504.0));

    return float4(result, 1.0f);
}