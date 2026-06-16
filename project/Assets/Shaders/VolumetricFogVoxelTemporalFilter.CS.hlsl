#include "ShaderConstants.hlsli"

Texture3D<float4> gVoxelInjectCurrent : register(t0); // 現在のフレームのInjection結果
Texture3D<float4> gVoxelInjectHistory : register(t1); // 前フレームのTemporal Filter結果
RWTexture3D<float4> gVoxelInjectFiltered : register(u0); // 出力先（これをAccumulateに渡す）

SamplerState gLinearSampler : register(s0);

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<VolumetricFogSettings> gFogSettings : register(b2);

[numthreads(8, 8, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVoxelInjectFiltered.GetDimensions(width, height, depth);
    if (DTid.x >= width || DTid.y >= height || DTid.z >= depth)
        return;

    float3 uvw = (float3(DTid) + 0.5f) / float3(width, height, depth);
    float4 currentFog = gVoxelInjectCurrent.Load(int4(DTid, 0));

    // 1. このボクセルの現在のワールド座標を計算
    float nearZ = max(gFrameData.nearClip, 0.1f);
    float farZ = min(gFrameData.farClip, gFogSettings.maxDistance);
    float viewZ = nearZ * pow(farZ / nearZ, uvw.z);
    
    float clipX = uvw.x * 2.0f - 1.0f;
    float clipY = (1.0f - uvw.y) * 2.0f - 1.0f;
    
    // ビューレイの方向からワールド座標を導出
    float4 worldTarget = mul(float4(clipX, clipY, 1.0f, 1.0f), gFrameData.invViewProj);
    float3 rayDir = normalize(worldTarget.xyz / worldTarget.w - gFrameData.cameraWorldPosition);
    float3 worldPos = gFrameData.cameraWorldPosition + rayDir * viewZ;

    // 2. 過去のフレームへリプロジェクション
    float4 prevClipPos = mul(float4(worldPos, 1.0f), gFrameData.prevViewProj);
    prevClipPos.xyz /= prevClipPos.w;
    
    float2 prevUV = prevClipPos.xy * float2(0.5f, -0.5f) + 0.5f;
    
    // Zの過去位置を再計算 (Exponential Depthに基づく逆計算)
    float prevViewZ = length(worldPos - gFrameData.prevCameraWorldPosition); // 厳密には深度軸での距離
    float prevZSlice = log2(prevViewZ / nearZ) / log2(farZ / nearZ);
    float3 prevUVW = float3(prevUV, prevZSlice);

    // サンプラーによる画面外のゴミ混入を防ぐため、安全にクランプ ===
    float3 clampedPrevUVW = saturate(prevUVW);
    bool isOffscreen = any(prevUVW < 0.0f) || any(prevUVW > 1.0f);

    // 完全に1.0にして履歴を捨てると生ジッターが爆発するので、
    // 画面外（初登場）でも30%程度に抑え、クランプした過去の滑らかなフォグと強引に混ぜる
    float blendAlpha = isOffscreen ? 0.3f : 0.05f;

    // 4. 過去の3Dボクセルをサンプリング
    float4 historyFog = gVoxelInjectHistory.SampleLevel(gLinearSampler, clampedPrevUVW, 0);

    // 5. ブレンド
    float4 result = lerp(historyFog, currentFog, blendAlpha);

    gVoxelInjectFiltered[DTid] = result;
}