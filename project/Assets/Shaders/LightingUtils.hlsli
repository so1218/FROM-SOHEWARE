float3 ApplyRimLight(
    float3 normal, float3 toEye, float3 toLight,
    float rimPower, int rimUseLightDir, float3 rimColor, float rimIntensity)
{
    // 基本のリムライト
    float NdotV = saturate(dot(normal, toEye));
    float rim = 1.0f - NdotV;
    rim = pow(rim, max(rimPower, 0.001f));

    // ライト方向によるマスク処理
    if (rimUseLightDir != 0)
    {
         // ライトが当たっている面 (NdotL) の強さを掛ける
        float NdotL = saturate(dot(normal, toLight));
        rim *= NdotL;
    }

    return rimColor * rim * rimIntensity;
}