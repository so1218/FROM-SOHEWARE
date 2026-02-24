#include "FullScreenQuad.hlsli"
#include "ShaderConstants.hlsli" 

ConstantBuffer <SSAOSettings>gSSAOSettings : register(b0);
ConstantBuffer<FrameData> gFrameData : register(b1);
Texture2D<float4> gNormalTexture : register(t0); // 描画済みの法線バッファ
Texture2D<float> gDepthTexture : register(t1); // 深度バッファ
SamplerState gClampSampler : register(s0); // はみ出し防止用クランプサンプラー

float LinearizeDepth(float depth, float nearClip, float farClip)
{
    return (nearClip * farClip) / (farClip - depth * (farClip - nearClip));
}

float3 GetViewPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 clipPos = float4(x, y, depth, 1.0f);
    float4 viewPos = mul(clipPos, gFrameData.invProjMatrix);
    return viewPos.xyz / viewPos.w;
}

// 完全にランダムではなく、画面空間で規則的で美しいノイズを生成
float InterleavedGradientNoise(float2 pixelPos)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelPos, magic.xy)));
}

float4 main(VSOutput input) : SV_TARGET
{
    float depth = gDepthTexture.SampleLevel(gClampSampler, input.uv, 0);
    
    if (depth >= 1.0f)
        return float4(1.0f, 1.0f, 1.0f, 1.0f);

    float3 worldNormal = gNormalTexture.SampleLevel(gClampSampler, input.uv, 0).xyz;
    float3 viewPos = GetViewPos(input.uv, depth);
    float3 viewNormal = normalize(mul(worldNormal, (float3x3) gFrameData.viewMatrix));

    // TBN行列の作成 (法線を中心とした半球を作るための準備)
    float noise = InterleavedGradientNoise(input.position.xy); // input.positionはピクセル座標
    float randomAngle = noise * 2.0f * 3.14159265f;
    float3 randomVec = float3(cos(randomAngle), sin(randomAngle), 0.0f);
    
    float3 tangent = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    float3 bitangent = cross(viewNormal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, viewNormal); // 法線(Z)を上に向ける行列

    float occlusion = 0.0f;
    int sampleCount = gSSAOSettings.sampleCount;

    // 半球サンプリング
    for (int i = 0; i < sampleCount; ++i)
    {
        // 規則正しいらせん状（スパイラル）にサンプリング点を配置
        float u = (float(i) + 0.5f) / float(sampleCount);
        float theta = u * 2.0f * 3.14159265f * 7.0f; // 7回転のらせん
        
        // 半球状の座標計算
        float r = sqrt(u); // 中心に偏らないように平方根
        float z = sqrt(max(0.0f, 1.0f - r * r)); // 高さを計算して半球
        float3 hemispherePos = float3(r * cos(theta), r * sin(theta), z);

        // TBN行列を掛けて、サンプリング点を法線の方向へ傾ける
        float3 sampleOffset = mul(hemispherePos, TBN);
        
        // 距離のスケール (中心に近いほど重みをつける)
        sampleOffset *= lerp(0.1f, 1.0f, u * u);

        float3 samplePos = viewPos + sampleOffset * gSSAOSettings.radius;

        // サンプル点をUVに変換
        float4 offsetPos = float4(samplePos, 1.0f);
        offsetPos = mul(offsetPos, gFrameData.projectionMatrix);
        offsetPos.xyz /= offsetPos.w;
        float2 sampleUV = float2(offsetPos.x * 0.5f + 0.5f, 1.0f - (offsetPos.y * 0.5f + 0.5f));

        // 画面外のサンプリングを無視する
        if (sampleUV.x < 0.0f || sampleUV.x > 1.0f || sampleUV.y < 0.0f || sampleUV.y > 1.0f)
            continue;

        float sampleDepth = gDepthTexture.SampleLevel(gClampSampler, sampleUV, 0);
        float sampleZ = GetViewPos(sampleUV, sampleDepth).z;

        // 遮蔽判定 (滑らかに減衰させる)
        float rangeCheck = smoothstep(0.0f, 1.0f, gSSAOSettings.radius / abs(viewPos.z - sampleZ));
        if (sampleZ < samplePos.z - gSSAOSettings.bias)
        {
            occlusion += 1.0f * rangeCheck;
        }
    }

    occlusion = 1.0f - (occlusion / (float) sampleCount);
    
    // 遠距離でフェードアウトさせる処理
    float linearDepth = LinearizeDepth(depth, gFrameData.nearClip, gFrameData.farClip);
    float fade = saturate((linearDepth - gSSAOSettings.fadeStart) / (gSSAOSettings.fadeEnd - gSSAOSettings.fadeStart));
    occlusion = lerp(occlusion, 1.0f, fade); // 遠くは影なし(1.0)

    occlusion = pow(abs(occlusion), gSSAOSettings.intensity);

    return float4(occlusion, occlusion, occlusion, 1.0f);
}