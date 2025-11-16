#ifdef __cplusplus
#include "MathUtils.h" 
#include "WorldTransform.h"

#define float4x4 Matrix4x4
#define float4 Vector4
#define float3 Vector3
#define float2 Vector2
#define int32_t int32_t
#define uint32_t uint32_t

#else

#endif

struct Material
{
    float4x4 uvTransform;

    float4 color;

    int32_t enableLighting;
    int32_t lightMode;
    int32_t isArtWave;
    int32_t isArtSound;

    int32_t isArtQuad;
    int32_t isArtKikagaku;
    int32_t isArtFrag;
    int32_t isArtGrid;

    float2 iResolution;
    float gTime;
    float shininess;

    float4 specularColor;

    float environmentMapIntensity;
    float3 padding;
};
