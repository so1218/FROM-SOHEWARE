#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli"

Texture2D gSceneTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);

ConstantBuffer<GodRaySettings> gGodRaySettings : register(b0);

float4 main(VSOutput input) : SV_TARGET
{
    float2 texCoord = input.uv;
    
    // オクルージョンマスクの作成(初期カラー)
    float depth = gDepthTexture.Sample(gSampler, texCoord);
    float4 color = gSceneTexture.Sample(gSampler, texCoord);
    
    // 簡易的なマスク(深度が1.0未満なら黒にする)
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

    // 光源に向かってサンプリング位置をずらしながら加算
    for (int i = 0; i < gGodRaySettings.numSamples; i++)
    {
        texCoord -= deltaTexCoord;
        
        // ずらした位置の深度と色をサンプリング
        float sampleDepth = gDepthTexture.Sample(gSampler, texCoord);
        float4 sampleColor = gSceneTexture.Sample(gSampler, texCoord);
        
        // マスク処理（同様に遮蔽物を黒にする）
        if (sampleDepth < 0.999f)
        {
            sampleColor = float4(0, 0, 0, 0);
        }
        else
        {
             // 閾値処理
            float luminance = dot(sampleColor.rgb, float3(0.2126, 0.7152, 0.0722));
            sampleColor.rgb *= step(gGodRaySettings.threshold, luminance);
        }

        sampleColor *= illuminationDecay * gGodRaySettings.weight;
        finalColor += sampleColor;
        illuminationDecay *= gGodRaySettings.decay;
    }

    return finalColor * gGodRaySettings.exposure;

}
