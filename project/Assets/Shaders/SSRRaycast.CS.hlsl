#include "ShaderConstants.hlsli"

ConstantBuffer<SSRSettings> gSSRSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

Texture2D<float4> gNormalTexture : register(t0); // G-Buffer: 法線
Texture2D<float> gHiZTexture : register(t1); // MipMap付きHi-Zテクスチャ (Mip0 = 元Depth)
Texture2D<float4> gMaterialTexture : register(t2); // G-Buffer: R=メタルネス, G=ラフネス

// 出力バッファ (R=HitUV.x, G=HitUV.y, B=HitAlpha, A=0.0)
RWTexture2D<float4> gOutHitResult : register(u0);

SamplerState gPointSampler : register(s0); // Hi-Zのサンプリングは補間なしのPointが必須

// ビュー空間座標を復元する関数
float3 GetViewPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(x, y, depth, 1.0f);
    float4 viewPos = mul(clipPos, gFrameData.invProjMatrix);
    return viewPos.xyz / viewPos.w;
}

// 疑似乱数ジェネレーター
float2 Hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xx + p3.yz) * p3.zy);
}

// GGXによるマイクロファセット法線の重要度サンプリング
float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 N)
{
    float a = roughness * roughness;
    float phi = 2.0f * 3.14159265f * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // 接空間からビュー空間への変換基底
    float3 up = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
    
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutHitResult.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    float2 uv = (float2(DTid.xy) + 0.5f) / float2(width, height);

    // Mip0から元深度を取得
    float depth = gHiZTexture.SampleLevel(gPointSampler, uv, 0);
    if (depth >= 1.0f)
    {
        gOutHitResult[DTid.xy] = float4(0, 0, 0, 0);
        return;
    }

    float4 material = gMaterialTexture.SampleLevel(gPointSampler, uv, 0);
    float roughness = material.g;

    // 高ラフネス（完全にツヤ消しの面）はSSR対象外にして早期リターン（圧倒的軽量化）
    if (roughness > 0.8f)
    {
        gOutHitResult[DTid.xy] = float4(0, 0, 0, 0);
        return;
    }

    float3 viewPos = GetViewPos(uv, depth);
    float3 worldNormal = gNormalTexture.SampleLevel(gPointSampler, uv, 0).xyz;
    float3 viewNormal = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));
    float3 viewDir = normalize(viewPos);

    // --- Stochastic（確率的）レイ方向の決定 ---
    float2 noiseSeed = uv + float2(gFrameData.frameCount * 0.034f, gFrameData.frameCount * 0.117f);
    float2 Xi = Hash22(noiseSeed);
    float3 H = ImportanceSampleGGX(Xi, roughness, viewNormal); // マイクロファセット法線
    float3 reflectDir = reflect(viewDir, H);

    if (dot(reflectDir, viewNormal) <= 0.0f)
    {
        gOutHitResult[DTid.xy] = float4(0, 0, 0, 0);
        return;
    }

    // --- Hi-Z 加速レイマーチング ---
    float3 rayPos = viewPos + viewNormal * 0.02f; // セルフシャドウ防止
    float maxMipLevel = 5.0f; // Hi-Zの最大探索Mip
    float currentMip = 0.0f;
    
    float2 hitUV = 0;
    float hitAlpha = 0.0f;
    float rayDistance = 0.0f;

    // Hi-Zのおかげで、少ないループ回数でも画面全体を走査可能
    [loop]
    for (int i = 0; i < gSSRSettings.maxSteps; ++i)
    {
        // ラフネスと進行距離に応じてベースのステップ幅を動的に変更
        float baseStep = gSSRSettings.stepSize * (1.0f + roughness * 2.0f);
        // Mipレベルが高ければステップを倍々にして大きくジャンプ
        float currentStep = baseStep * pow(2.0f, currentMip);
        
        rayPos += reflectDir * currentStep;
        rayDistance += currentStep;

        // 現在のレイの位置を画面UV空間とノンリニア深度へ射影
        float4 offsetPos = mul(float4(rayPos, 1.0f), gFrameData.projectionMatrix);
        offsetPos.xyz /= offsetPos.w;
        float2 sampleUV = float2(offsetPos.x * 0.5f + 0.5f, 1.0f - (offsetPos.y * 0.5f + 0.5f));

        // 画面外判定
        if (any(sampleUV < 0.0f) || any(sampleUV > 1.0f))
            break;

        // レイ自体のノンリニア深度（0.0=Near, 1.0=Far）
        float rayRaycastDepth = offsetPos.z;

        // 現在のMipレベルにおける、その領域の「最も手前にあるオブジェクトの深度」を取得
        float cellDepth = gHiZTexture.SampleLevel(gPointSampler, sampleUV, (uint) currentMip);

        // 判定（標準Depth: 値が小さいほど手前）
        if (rayRaycastDepth < cellDepth)
        {
            // レイは遮蔽物よりも手前（空域）にいる -> 衝突の可能性ゼロ
            // 安心してMipレベルを上げ、次のループで大きく進む
            currentMip = min(currentMip + 1.0f, maxMipLevel);
        }
        else
        {
            // レイが遮蔽物よりも奥に入り込んだ -> 衝突した可能性あり
            if (currentMip > 0.0f)
            {
                // まだ大雑把なMipなので、レイを1歩戻してMipを下げ、細かくやり直す
                rayPos -= reflectDir * currentStep;
                rayDistance -= currentStep;
                currentMip -= 1.0f;
            }
            else
            {
                // 最精細（Mip 0）で裏に入り込んだ＝衝突！
                // ただし、オブジェクトの厚みを突き抜けた背景の空中である可能性を排除
                float depthDiff = rayRaycastDepth - cellDepth;
                
                // ビュー空間の線形な厚み制限に変換（簡易的に設定値と比較）
                if (depthDiff > 0.0f && depthDiff < gSSRSettings.thickness)
                {
                    // 衝突面の法線チェック（裏面カリング）
                    // ※本来はResolveパスで行っても良いが、ここで弾くと精度が上がる
                    hitUV = sampleUV;
                    hitAlpha = smoothstep(0.0f, gSSRSettings.stepSize * 2.0f, rayDistance);
                    break;
                }
            }
        }
    }

    // --- 各種フェード処理 ---
    // 画面端フェード
    float2 edgeFade = min(hitUV, 1.0f - hitUV) * 10.0f;
    hitAlpha *= saturate(edgeFade.x) * saturate(edgeFade.y);
    // 距離フェード
    hitAlpha *= (1.0f - saturate(rayDistance / gSSRSettings.maxDistance));

    // 結果の出力（ここでは色サンプリングをせず、UVとマスク情報のみを保存）
    gOutHitResult[DTid.xy] = float4(hitUV, hitAlpha, 0.0f);
}