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
    
    // ★重要: 固定の「光の色」を決める（定数バッファから取るのが理想）
    float3 lightColor = gGodRaySettings.lightColor;
    for (int i = 0; i < gGodRaySettings.numSamples; i++)
    {
        texCoord -= deltaTexCoord;

        // 画面外チェック（前回のBORDER設定をしていない場合の保険）
        if (any(step(1.0, texCoord) + step(texCoord, 0.0)))
            break;

        // 深度をサンプリング
        float sampleDepth = gDepthTexture.Sample(gSampler, texCoord);

        // ★修正点: シーンカラー(gSceneTexture)は一切読み込まない！
        // 代わりに「深度」だけで「光」か「影」かを判定する。

// シーンカラーをサンプリング
        float4 sceneColor = gSceneTexture.Sample(gSampler, texCoord);

        // ★修正点2: 深度による分岐を削除し、輝度のみで判定する
        
        // 輝度を計算
        float luminance = dot(sceneColor.rgb, float3(0.299, 0.587, 0.114));

        float3 sampleColor = float3(0, 0, 0);

        // 「輝度が閾値を超えている」なら、それは距離に関係なく「光」とする
        if (luminance > gGodRaySettings.threshold)
        {
            // 光源の色（またはシーンの色を使ってもOK）
            sampleColor = lightColor;
            
            // もし「パーティクルやオブジェクトの色」をそのままゴッドレイの色にしたい場合はこちら
            // sampleColor = sceneColor.rgb; 
        }
        else
        {
            // 暗いものは「遮蔽物」として黒にする
            sampleColor = float3(0, 0, 0);
        }
        // --- ここから下はパーティクル対策 ---
        // パーティクルが「深度を書き込まない(Z-Write Off)」設定であれば、
        // 深度バッファには「パーティクルの奥にある空」の値が入っているため、
        // 上記の判定だけでパーティクルを無視して光が貫通します。
        
        // 加算
        sampleColor *= illuminationDecay * gGodRaySettings.weight;
        finalColor.rgb += sampleColor;
        illuminationDecay *= gGodRaySettings.decay;
    }

    return finalColor * gGodRaySettings.exposure;
}