#define MAX_DIRECTIONAL_LIGHTS 2
#define MAX_POINT_LIGHTS 4
#define MAX_SPOT_LIGHTS 2

struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t3 worldPosition : POSITION1;
};

struct DirectionalLight 
{
    float4 color;
    float3 direction;
    float intensity;
    int enable;
};

struct Camera
{
    float32_t3 worldPosition;
    float32_t padding0;
};

struct PointLight
{
    float32_t4 color; 
    float32_t3 position;
    float intensity;
    float radius;
    float decay;
    int enable;
};

struct SpotLight
{
    float32_t4 color;
    float32_t3 position;
    float32_t intensity;
    float32_t3 direction;
    float32_t distance;
    float32_t decay;
    float32_t cosAngle;
    int enable;
};

// Light types
#define LIGHT_HALFLAMBERT 0
#define LIGHT_PHONG_SPECULAR 1
#define LIGHT_TOON 2
#define LIGHT_POINT 3
#define LIGHT_SPOT 4