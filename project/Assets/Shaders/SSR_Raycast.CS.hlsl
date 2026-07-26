#include "ShaderConstants.hlsli"

ConstantBuffer<SSRSettings> gSSRSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);

Texture2D<float4> gNormalTexture : register(t0);
Texture2D<float> gHiZTexture : register(t1); // Hierarchical Z-Buffer (Mip0 = Base Depth)
Texture2D<float4> gMaterialTexture : register(t2); // R=Metallic, G=Roughness

RWTexture2D<float4> gOutHitResult : register(u0); // R=HitUV.x, G=HitUV.y, B=HitAlpha, A=0.0
SamplerState gPointSampler : register(s0); // Hi-Zトラバーサル時の補間回避用

// クリップ空間からビュー空間への座標復元
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

    // 背景(Skybox等)のリジェクト
    float depth = gHiZTexture.SampleLevel(gPointSampler, uv, 0);
    if (depth >= 1.0f)
    {
        gOutHitResult[DTid.xy] = float4(0, 0, 0, 0);
        return;
    }

    float4 material = gMaterialTexture.SampleLevel(gPointSampler, uv, 0);
    float roughness = material.g;

    // 粗いマテリアルのトレース省略 (パフォーマンス最適化)
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
    
    // レイがカメラ側(手前)を向いている場合は早期リターン
    if (dot(reflectDir, viewNormal) <= 0.001f)
    {
        gOutHitResult[DTid.xy] = float4(0, 0, 0, 0);
        return;
    }
    
    // Ray Setup (View Space -> Screen Space)
    // セルフルイティング回避のため、レイの始点を法線/視線方向にオフセット
    float3 rayStartView = viewPos + (viewNormal * 0.05f) - (viewDir * 0.05f);
    float3 rayEndView = rayStartView + reflectDir * gSSRSettings.maxDistance;

    // ニアプレーン交差クリップ (Z < 0 突き抜け防止)
    if (rayEndView.z < 0.1f)
    {
        float tClip = (0.1f - rayStartView.z) / (rayEndView.z - rayStartView.z);
        rayEndView = lerp(rayStartView, rayEndView, tClip);
    }

    float4 rStartClip = mul(float4(rayStartView, 1.0f), gFrameData.projectionMatrix);
    float4 rEndClip = mul(float4(rayEndView, 1.0f), gFrameData.projectionMatrix);

    float3 rStartScreen = rStartClip.xyz / rStartClip.w;
    float3 rEndScreen = rEndClip.xyz / rEndClip.w;

    float3 uv0 = float3(rStartScreen.x * 0.5f + 0.5f, 1.0f - (rStartScreen.y * 0.5f + 0.5f), rStartScreen.z);
    float3 uv1 = float3(rEndScreen.x * 0.5f + 0.5f, 1.0f - (rEndScreen.y * 0.5f + 0.5f), rEndScreen.z);
    
    float3 dUVZ = uv1 - uv0;

    // Hi-Z Cell-Crossing DDA Traversal
    float maxMipLevel = 5.0f;
    float currentMip = 0.0f;
    
    float t = 0.0f;
    float t_prev = 0.0f;
    
    float2 texelSize = 1.0f / float2(width, height);
    float2 uvDir = normalize(dUVZ.xy);
    
    // 自身のピクセルでの誤ヒットを避けるため始点を進める
    uv0.xy += uvDir * texelSize * 1.5f;
    float3 currentUVZ = uv0;

    // ゼロ除算回避用の安全な方向ベクトル
    float2 signDir = float2(dUVZ.x >= 0.0f ? 1.0f : -1.0f, dUVZ.y >= 0.0f ? 1.0f : -1.0f);
    float2 safe_dUV = dUVZ.xy + (signDir * 1e-8f);

    float2 hitUV = 0;
    float hitAlpha = 0.0f;
    float rayDistance = 0.0f;

    [loop]
    for (int i = 0; i < gSSRSettings.maxSteps; ++i)
    {
        if (any(currentUVZ.xy < 0.0f) || any(currentUVZ.xy > 1.0f) || t > 1.0f)
            break;

        uint2 mipSize = uint2(width, height) >> (uint) currentMip;
        
        // DDAセル境界計算 (浮動小数点誤差によるスタック防止のため微小オフセットを加算)
        float2 cellBoundary = (floor(currentUVZ.xy * mipSize) + max(signDir, 0.0f) + signDir * 0.0001f) / mipSize;
        
        // レイが現在のセル境界を跨ぐパラメーター(t)を算出
        float2 tValues = (cellBoundary - currentUVZ.xy) / safe_dUV;
        float tMax = min(tValues.x, tValues.y);
        float tNext = t + tMax;

        float cellDepth = gHiZTexture.SampleLevel(gPointSampler, currentUVZ.xy, (uint) currentMip);

        if (currentUVZ.z < cellDepth)
        {
            // レイがセルの手前(空域)にある -> 安全にスキップしてMipレベルを上げる
            t_prev = t;
            t = tNext;
            currentUVZ = uv0 + t * dUVZ;
            
            currentMip = min(currentMip + 1.0f, maxMipLevel);
        }
        else
        {
            // セル奥へレイが貫通 -> 交差の可能性があるため詳細化
            if (currentMip > 0.0f)
            {
                currentMip -= 1.0f;
            }
            else
            {
                // Mip0での交差判定: ビュー空間での線形深度差を利用した厚みテスト
                float rayLinearZ = GetViewPos(currentUVZ.xy, currentUVZ.z).z;
                float cellLinearZ = GetViewPos(currentUVZ.xy, cellDepth).z;
                float linearDepthDiff = rayLinearZ - cellLinearZ;
                
                float dynamicThickness = gSSRSettings.thickness + (t * 0.1f); // 進行距離に応じた厚みの緩和

                if (linearDepthDiff > 0.0f && linearDepthDiff < dynamicThickness)
                {
                    // 交差点のサブピクセル精度向上 (二分探索)
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

                    float3 finalUVZ = uv0 + tMax * dUVZ;
                    hitUV = finalUVZ.xy;
                    
                    float3 hitViewPosFinal = lerp(rayStartView, rayEndView, tMax);
                    rayDistance = distance(hitViewPosFinal, viewPos);

                    hitAlpha = smoothstep(0.0f, gSSRSettings.stepSize * 2.0f, rayDistance);
                    break;
                }
                else
                {
                    // オブジェクトの背後(厚み以上)を完全に突き抜けた -> スキップ
                    t_prev = t;
                    t = tNext;
                    currentUVZ = uv0 + t * dUVZ;
                }
            }
        }
    }

    // フェード処理
    // 画面端の急な途切れを隠すスクリーンエッジフェード
    float2 edgeFade = min(hitUV, 1.0f - hitUV) * 10.0f;
    hitAlpha *= saturate(edgeFade.x) * saturate(edgeFade.y);
    
    // レイの移動距離フェード
    hitAlpha *= (1.0f - saturate(rayDistance / gSSRSettings.maxDistance));

    gOutHitResult[DTid.xy] = float4(hitUV, hitAlpha, 0.0f);
}