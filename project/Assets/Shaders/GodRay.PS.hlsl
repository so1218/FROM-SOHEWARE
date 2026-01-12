#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);

ConstantBuffer<GodRaySettings> gGodRaySettings : register(b0);

float4 main(VSOutput input) : SV_TARGET
{
    float2 texCoord = input.uv;
    
    // オクルージョンマスクの作成
    float depth = gDepthTexture.Sample(gSampler, texCoord);
    float4 color = gSceneTexture.Sample(gSampler, texCoord);
    
    // マスク処理(深度が1.0未満なら黒にする)
    if(depth<0.999f)
    {
        color = float4(0, 0, 0, 0);
    }
    else
    {
        // 輝度闘値処理
        float luminance = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
        color = float4(color.rgb * step(gGodRaySettings.threshold, luminance), 1.0f);
    }
    
    // ラディアルブラー
    float2 deltaTexCoord = (texCoord - gGodRaySettings.lightPosScreen);
    
    // サンプリングステップ幅の計算
    deltaTexCoord *= 1.0f / float(gGodRaySettings.numSamples) * gGodRaySettings.density;

    float illuminationDecay = 1.0f;
    float4 finalColor = float4(0, 0, 0, 0);
    
    // 光の色
    float3 lightColor = gGodRaySettings.lightColor;
    
    for (int i = 0; i < gGodRaySettings.numSamples; i++)
    {
        texCoord -= deltaTexCoord;

        // 画面外チェック
        if (any(step(1.0, texCoord) + step(texCoord, 0.0)))
            break;

        // 深度をサンプリング
        float sampleDepth = gDepthTexture.Sample(gSampler, texCoord);

        // シーンカラーをサンプリング
        float4 sceneColor = gSceneTexture.Sample(gSampler, texCoord);

        // 輝度を計算
        float luminance = dot(sceneColor.rgb, float3(0.299, 0.587, 0.114));

        float3 sampleColor = float3(0, 0, 0);

        // 輝度が閾値を超えているなら、それは距離に関係なく光
        if (luminance > gGodRaySettings.threshold)
        {
            // 光源の色
            sampleColor = lightColor;
        }
        else
        {
            // 暗いものは遮蔽物として黒にする
            sampleColor = float3(0, 0, 0);
        }
        
        // 加算
        sampleColor *= illuminationDecay * gGodRaySettings.weight;
        finalColor.rgb += sampleColor;
        illuminationDecay *= gGodRaySettings.decay;
    }

    return finalColor * gGodRaySettings.exposure;
}