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
};

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t3 smoothNormal : TANGENT0;
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
    float32_t3 smoothNormal;
};

// Light types
#define LIGHT_HALFLAMBERT 0
#define LIGHT_PHONG_SPECULAR 1
#define LIGHT_TOON 2
#define LIGHT_POINT 3
#define LIGHT_SPOT 4