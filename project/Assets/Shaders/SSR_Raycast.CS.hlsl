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

    if (roughness > 0.8f)
    {
        gOutHitResult[DTid.xy] = float4(0, 0, 0, 0);
        return;
    }

    float3 viewPos = GetViewPos(uv, depth);
    float3 worldNormal = gNormalTexture.SampleLevel(gPointSampler, uv, 0).xyz;
    float3 viewNormal = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));
    float3 viewDir = normalize(viewPos);

    float3 reflectDir = reflect(viewDir, viewNormal);

    // 常に視線と法線から、完璧な鏡の反射ベクトル(1本)を作る
    float NdotL = dot(reflectDir, viewNormal);
    
    if (NdotL <= 0.001f)
    {
        gOutHitResult[DTid.xy] = float4(0, 0, 0, 0);
        return;
    }
    
    // 2D Cell-Crossing DDA レイマーチング
    
    // 3Dビュー空間でのレイの開始点と終了点を定義
    float3 rayStartView = viewPos + (viewNormal * 0.05f) - (viewDir * 0.05f);
    float3 rayEndView = rayStartView + reflectDir * gSSRSettings.maxDistance;

    // カメラの後ろ（ニアプレーンより手前）にレイが突き抜けるのを防ぐクリップ
    if (rayEndView.z < 0.1f)
    {
        float tClip = (0.1f - rayStartView.z) / (rayEndView.z - rayStartView.z);
        rayEndView = lerp(rayStartView, rayEndView, tClip);
    }

    // 3D空間のレイの端点を画面空間（UV + ノンリニア深度）へ一度だけ投影
    float4 rStartClip = mul(float4(rayStartView, 1.0f), gFrameData.projectionMatrix);
    float4 rEndClip = mul(float4(rayEndView, 1.0f), gFrameData.projectionMatrix);

    float3 rStartScreen = rStartClip.xyz / rStartClip.w;
    float3 rEndScreen = rEndClip.xyz / rEndClip.w;

    // NDC座標 (-1~1) から UV空間 (0~1) への変換、およびノンリニア深度の抽出
    float3 uv0 = float3(rStartScreen.x * 0.5f + 0.5f, 1.0f - (rStartScreen.y * 0.5f + 0.5f), rStartScreen.z);
    float3 uv1 = float3(rEndScreen.x * 0.5f + 0.5f, 1.0f - (rEndScreen.y * 0.5f + 0.5f), rEndScreen.z);

    // 2D画面空間上でのレイの方向ベクトル（始点から終点への差分）
    float3 dUVZ = uv1 - uv0;

    float maxMipLevel = 5.0f; // Hi-Zの最大探索Mip
    float currentMip = 0.0f;
    
    // レイの進行度パラメータ t (0.0 = 始点, 1.0 = 終点)
    float t = 0.0f;
    float t_prev = 0.0f;
    
    float2 texelSize = 1.0f / float2(width, height);
// レイの画面空間での移動方向(正規化)
    float2 uvDir = normalize(dUVZ.xy);
// 1ピクセル分だけ始点を進める
    uv0.xy += uvDir * texelSize * 1.5f;
    
    float3 currentUVZ = uv0;

    // 0除算によるレイ走査のフリーズ(NaN)を防ぐための、安全な方向符号とベクトル
    float2 signDir = float2(dUVZ.x >= 0.0f ? 1.0f : -1.0f, dUVZ.y >= 0.0f ? 1.0f : -1.0f);
    float2 safe_dUV = dUVZ.xy + (signDir * 1e-8f);

    float2 hitUV = 0;
    float hitAlpha = 0.0f;
    float rayDistance = 0.0f;

    [loop]
    for (int i = 0; i < gSSRSettings.maxSteps; ++i)
    {
        // 画面外、またはレイの終点(t=1.0)に達したら探索終了
        if (any(currentUVZ.xy < 0.0f) || any(currentUVZ.xy > 1.0f) || t > 1.0f)
            break;

        // 現在のMip解像度におけるセルのサイズ（ピクセル数）を取得
        uint2 mipSize = uint2(width, height) >> (uint) currentMip;
        
        // セルの境界線を進行方向に0.05ピクセル分わずかにずらす。
        // これにより、浮動小数点誤差で境界を跨いだ直後に同じセルにスタックして無限ループする現象を完璧に防ぐ
        float2 cellBoundary = (floor(currentUVZ.xy * mipSize) + max(signDir, 0.0f) + signDir * 0.0001f) / mipSize;
        
        // 現在位置からセルの境界線に到達するまでの t の増分を計算し、小さい（手前にある）方の軸を選択
        float2 tValues = (cellBoundary - currentUVZ.xy) / safe_dUV;
        float tMax = min(tValues.x, tValues.y);
        float tNext = t + tMax;

        // 現在の位置が属するセルのHi-Z深度を取得
        float cellDepth = gHiZTexture.SampleLevel(gPointSampler, currentUVZ.xy, (uint) currentMip);

        // 深度判定（標準Depth: 値が小さいほどカメラに近く、手前）
        if (currentUVZ.z < cellDepth)
        {
            // レイの深度がセルの最手前深度より手前（空域）にある -> このセルには絶対衝突しない！
            // 安心してこのセルをスキップし、次のセルの境界（tNext）まで一瞬でワープ
            t_prev = t;
            t = tNext;
            currentUVZ = uv0 + t * dUVZ;
            
            // スキップに成功したので、さらに効率よく進むためにMipレベルを上げる
            currentMip = min(currentMip + 1.0f, maxMipLevel);
        }
        else
        {
            // セルの最手前深度より奥（遮蔽物の裏側）に入り込んだ -> 衝突の可能性あり
            if (currentMip > 0.0f)
            {
                // まだ粗いMipレベルなので、Mipを1つ下げてセルを細分化し、同じ位置(t)から詳細に調べ直す
                currentMip -= 1.0f;
            }
            else
            {
                float rayLinearZ = GetViewPos(currentUVZ.xy, currentUVZ.z).z;
                float cellLinearZ = GetViewPos(currentUVZ.xy, cellDepth).z;
                float linearDepthDiff = rayLinearZ - cellLinearZ;
                
                // レイの移動距離（t）に応じて少しだけ厚みを許容する安定した計算へ
                float dynamicThickness = gSSRSettings.thickness + (t * 0.1f); // 距離に応じて微増

                if (linearDepthDiff > 0.0f && linearDepthDiff < dynamicThickness)
                {
                    // 二分探索の精度向上 (4回 -> 8回)
                    // 浅い角度での縞々を消すためには回数が命
                    float tMin = t_prev;
                    float tMax = t;
                    
                    [unroll]
                    for (int b = 0; b < 8; ++b)
                    {
                        float tMid = lerp(tMin, tMax, 0.5f);
                        float3 midUVZ = uv0 + tMid * dUVZ;
                        float midCellDepth = gHiZTexture.SampleLevel(gPointSampler, midUVZ.xy, 0);
                        
                        if (midUVZ.z > midCellDepth)
                            tMax = tMid;
                        else
                            tMin = tMid;
                    }

                    // 最終的に求まった高精度な交点UVを採用
                    float3 finalUVZ = uv0 + tMax * dUVZ;
                    hitUV = finalUVZ.xy;
                    
                    // フェード用の3D空間進行距離を逆算
                    float3 hitViewPosFinal = lerp(rayStartView, rayEndView, tMax);
                    rayDistance = distance(hitViewPosFinal, viewPos);

                    hitAlpha = smoothstep(0.0f, gSSRSettings.stepSize * 2.0f, rayDistance);
                    break; // トレース成功、ループを抜ける
                }
                else
                {
                    // 厚みを超えてオブジェクトを完全に突き抜けた
                    // 衝突しなかったので、このMip0セルをスキップして次のセルへ進む
                    t_prev = t;
                    t = tNext;
                    currentUVZ = uv0 + t * dUVZ;
                }
            }
        }
    }

    // 各種フェード処理
    // 画面端フェード
    float2 edgeFade = min(hitUV, 1.0f - hitUV) * 10.0f;
    hitAlpha *= saturate(edgeFade.x) * saturate(edgeFade.y);
    // 距離フェード
    hitAlpha *= (1.0f - saturate(rayDistance / gSSRSettings.maxDistance));

    // 結果の出力
    gOutHitResult[DTid.xy] = float4(hitUV, hitAlpha, 0.0f);
}