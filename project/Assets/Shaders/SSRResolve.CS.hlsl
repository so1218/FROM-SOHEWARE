#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);

// 入力テクスチャ
Texture2D<float4> gHitResultTexture : register(t0); // 先ほど作った HitUV (R,G) と HitAlpha (B)
Texture2D<float4> gSceneTexture : register(t1); // 反射元のカラー（前フレームまたは現フレームの不透明描画結果）
Texture2D<float4> gNormalTexture : register(t2); // G-Buffer: 法線
Texture2D<float> gDepthTexture : register(t3); // G-Buffer: 深度
Texture2D<float4> gMaterialTexture : register(t4); // G-Buffer: R=メタルネス, G=ラフネス

// 出力テクスチャ (最終的な反射カラー)
RWTexture2D<float4> gOutReflection : register(u0);

SamplerState gLinearSampler : register(s0);

// ビュー空間座標の復元（Raycastと同じ）
float3 GetViewPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(x, y, depth, 1.0f);
    float4 viewPos = mul(clipPos, gFrameData.invProjMatrix);
    return viewPos.xyz / viewPos.w;
}

// フレネル・シュリック近似
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(max(1.0f - cosTheta, 0.0f), 5.0f);
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutReflection.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    float2 uv = (float2(DTid.xy) + 0.5f) / float2(width, height);

    // レイキャストの結果を取得
    float4 hitData = gHitResultTexture.Load(int3(DTid.xy, 0));
    float2 hitUV = hitData.xy;
    float hitAlpha = hitData.z;

    // Hitしていない場合は早期リターン
    if (hitAlpha <= 0.0f)
    {
        gOutReflection[DTid.xy] = float4(0, 0, 0, 0);
        return;
    }

    // 現在のピクセルの情報を取得
    float depth = gDepthTexture.Load(int3(DTid.xy, 0));
    float4 material = gMaterialTexture.Load(int3(DTid.xy, 0));
    float metalness = material.r;
    float roughness = material.g;
    float3 worldNormal = gNormalTexture.Load(int3(DTid.xy, 0)).xyz;
    
    // G-Buffer情報の復元
    float3 viewPos = GetViewPos(uv, depth);
    float3 viewNormal = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));
    float3 viewDir = normalize(-viewPos); // カメラからピクセルへの逆ベクトル

    // 基礎反射率 (F0) の計算
    float3 albedo = gSceneTexture.Load(int3(DTid.xy, 0)).rgb; // 必要に応じてアルベドバッファから取得
    float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);

    // フレネル項の計算
    float NdotV = max(dot(viewNormal, viewDir), 0.0f);
    float3 fresnel = FresnelSchlick(NdotV, f0);

    // ヒット先のカラーを取得 (Linearサンプリングで滑らかに)
    // ※本来は前のフレームのカラー(History Buffer)を使うのがベスト
    float3 hitColor = gSceneTexture.SampleLevel(gLinearSampler, hitUV, 0).rgb;

    // 最終カラー = 取得した色 * フレネル * マスク(Alpha)
    // Stochastic SSRの場合、ここの出力はノイズだらけ（それが正常）
    float3 finalReflection = hitColor * fresnel * hitAlpha;

    gOutReflection[DTid.xy] = float4(finalReflection, hitAlpha);
}