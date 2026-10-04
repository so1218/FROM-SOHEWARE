// ノーマルマップからワールド空間の法線を計算
float3 CalculateNormalFromMap(
    float3 vertexNormal, float3 vertexTangent, float2 uv,
    float normalIntensity, Texture2D<float4> normalTex, SamplerState texSampler)
{
    // ノーマルマップから法線をサンプリング
    float3 mapSample = normalTex.Sample(texSampler, uv).rgb;
    
    // (0,1)を(-1,1)に変換
    float3 mapNormal = mapSample * 2.0f - 1.0f;

    // 法線の強度調整
    mapNormal.xy *= normalIntensity;

    // TBN行列の構築と変換
    float3 N = normalize(vertexNormal);
    
    // グラム・シュミットの直交化を用いて接線を再直交化
    float3 T = normalize(vertexTangent - dot(vertexTangent, N) * N);
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);
    float3 transformedNormal = mul(mapNormal, TBN);

    return normalize(transformedNormal);
}
