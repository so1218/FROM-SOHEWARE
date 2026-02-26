#define MAX_DIRECTIONAL_LIGHTS 2
#define MAX_POINT_LIGHTS 4
#define MAX_SPOT_LIGHTS 2
#define MAX_AREA_LIGHTS 2

struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t3 worldPosition : POSITION1;
    float4 shadowCoord : POSITION2;
    float3 tangent : TANGENT;
    float32_t4 worldColor : COLOR0;
};

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION1;
    float4 shadowCoord : POSITION2;
    float3 tangent : TANGENT;
    float4 worldColor : COLOR0;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 material : SV_TARGET2;
};

struct Well
{
    float32_t4x4 skeletonSpaceMatrix;
    float32_t4x4 skeletonSpaceInverseTransposeMatrix;
};

struct Skinned
{
    float32_t4 position;
    float32_t3 normal;
    float32_t3 tangent;
    float32_t3 smoothNormal;
};

// Light types
#define SHADING_MODEL_HALFLAMBERT 0
#define SHADING_MODEL_PHONG 1
#define SHADING_MODEL_TOON 2
#define SHADING_MODEL_PBR 3
#define LIGHT_POINT 4
#define LIGHT_SPOT 5

static const float PI = 3.14159265359f;
static const float EPSILON = 0.00001f;
