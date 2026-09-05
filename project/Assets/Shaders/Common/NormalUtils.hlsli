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

// パララックスオクルージョンマッピング
float2 CalculateParallaxOcclusionMapping(
    float2 texCoords, float3 viewDirTS, float2 dx, float2 dy, float pomHeightScale, float pomMaxSteps,
    float pomMinSteps, Texture2D<float> heightMap, SamplerState texSampler, out float parallaxHeight)
{
    viewDirTS = normalize(viewDirTS);

    // カメラがサーフェスの裏側にある場合は早期リターン
    if (viewDirTS.z <= 0.0f)
    {
        parallaxHeight = 0.0f;
        return texCoords;
    }

    float mipLevel = heightMap.CalculateLevelOfDetail(texSampler, texCoords);

    // パラメータの安全化
    float safeHeightScale = clamp(pomHeightScale, 0.0f, 0.1f);
    float uvScale = length(float2(dx.x, dy.y)) * 1024.0f;
    safeHeightScale /= max(uvScale, 1.0f);

    float scaleFactor = safeHeightScale / 0.05f;
    float maxSteps = clamp(pomMaxSteps * scaleFactor, 16.0f, 128.0f);
    float minSteps = clamp(pomMinSteps * scaleFactor, 8.0f, 64.0f);

    float numSteps = lerp(maxSteps, minSteps, viewDirTS.z);
    float stepSize = 1.0f / numSteps;

    float2 parallaxDir = viewDirTS.xy / max(viewDirTS.z, 0.01f);
    
    float maxRatio = 1.5f;
    float currentRatio = length(parallaxDir);
    if (currentRatio > maxRatio)
    {
        parallaxDir *= (maxRatio / currentRatio);
    }

    float2 p = parallaxDir * safeHeightScale;
    float2 deltaTexCoords = p * stepSize;
    float2 currentTexCoords = texCoords;
    
    float currentLayerDepth = 0.0f;
    float currentDepthMapValue = 1.0f - heightMap.SampleLevel(texSampler, currentTexCoords, mipLevel).r;

    [unroll(128)]
    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentLayerDepth += stepSize;
        currentDepthMapValue = 1.0f - heightMap.SampleLevel(texSampler, currentTexCoords, mipLevel).r;
    }

    float2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (1.0f - heightMap.SampleLevel(texSampler, prevTexCoords, mipLevel).r) - currentLayerDepth + stepSize;

    float weight = afterDepth / (afterDepth - beforeDepth);
    float2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0f - weight);

    parallaxHeight = currentLayerDepth - stepSize * (1.0f - weight);

    return finalTexCoords;
}

// POMによるソフト自己影の計算
float CalculatePOMSoftShadow(
    float3 lightDirTS, float2 initialUV, float initialHeight, float2 dx, float2 dy, float pomHeightScale,
    float pomMaxSteps, float pomMinSteps, Texture2D<float> heightMap, SamplerState texSampler)
{
    lightDirTS = normalize(lightDirTS);

    if (lightDirTS.z <= 0.0f)
        return 0.0f;

    float mipLevel = heightMap.CalculateLevelOfDetail(texSampler, initialUV);

    float numSteps = lerp(pomMaxSteps, pomMinSteps, lightDirTS.z);
    float stepSize = 1.0f / numSteps;

    float2 parallaxDir = lightDirTS.xy / max(lightDirTS.z, 0.01f);
    float maxRatio = 1.5f;
    float currentRatio = length(parallaxDir);
    if (currentRatio > maxRatio)
    {
        parallaxDir *= (maxRatio / currentRatio);
    }

    float2 p = parallaxDir * pomHeightScale;
    float2 deltaTexCoords = p * stepSize;

    float2 currentTexCoords = initialUV;
    float currentLayerDepth = initialHeight - stepSize;
    float shadowMultiplier = 1.0f;

    // 実際のループ回数
    int maxShadowSteps = (int) numSteps;
    
    [loop]
    for (int i = 0; i < maxShadowSteps; ++i)
    {
        // 念のため深さが0以下になったら抜ける
        if (currentLayerDepth <= 0.0f)
            break;

        currentTexCoords += deltaTexCoords;
        // heightMapのサンプリング処理
        float currentDepthMapValue = 1.0f - heightMap.SampleLevel(texSampler, currentTexCoords, mipLevel).r;
        
        if (currentDepthMapValue < currentLayerDepth)
        {
            float currentShadow = (currentLayerDepth - currentDepthMapValue) * 4.0f;
            shadowMultiplier = min(shadowMultiplier, 1.0f - currentShadow);
        }
        currentLayerDepth -= stepSize;
    }

    return saturate(shadowMultiplier);
}

// カラーテクスチャ用トライプラナーマッピング
float4 CalculateTriplanarColor(
    float3 worldPos, float3 worldNormal, float texScale,
    float blendSharpness, Texture2D<float4> colorTex, SamplerState texSampler)
{
    float3 blendWeights = abs(worldNormal);
    blendWeights = pow(blendWeights, blendSharpness);
    blendWeights /= max(blendWeights.x + blendWeights.y + blendWeights.z, 0.0001f);

    float2 uvX = worldPos.zy * texScale;
    float2 uvY = worldPos.xz * texScale;
    float2 uvZ = worldPos.xy * texScale;

    float4 tX = colorTex.Sample(texSampler, uvX);
    float4 tY = colorTex.Sample(texSampler, uvY);
    float4 tZ = colorTex.Sample(texSampler, uvZ);

    return tX * blendWeights.x + tY * blendWeights.y + tZ * blendWeights.z;
}

// ノーマルマップ用トライプラナーマッピング
float3 CalculateTriplanarNormal(
    float3 worldPos, float3 worldNormal, float texScale,
    float blendSharpness, Texture2D<float4> normalTex, SamplerState texSampler)
{
    float3 blendWeights = abs(worldNormal);
    blendWeights = pow(blendWeights, blendSharpness);
    blendWeights /= max(blendWeights.x + blendWeights.y + blendWeights.z, 0.0001f);

    float2 uvX = worldPos.zy * texScale;
    float2 uvY = worldPos.xz * texScale;
    float2 uvZ = worldPos.xy * texScale;

    // .xyz を抽出して -1.0 ~ 1.0 に変換
    float3 tX = normalTex.Sample(texSampler, uvX).xyz * 2.0f - 1.0f;
    float3 tY = normalTex.Sample(texSampler, uvY).xyz * 2.0f - 1.0f;
    float3 tZ = normalTex.Sample(texSampler, uvZ).xyz * 2.0f - 1.0f;

    float3 nX = float3(tX.z * sign(worldNormal.x), tX.y, -tX.x);
    float3 nY = float3(tY.x, tY.z * sign(worldNormal.y), -tY.y);
    float3 nZ = float3(tZ.x, tZ.y, tZ.z * sign(worldNormal.z));

    float3 finalNormal = nX * blendWeights.x + nY * blendWeights.y + nZ * blendWeights.z;

    return normalize(finalNormal);
}