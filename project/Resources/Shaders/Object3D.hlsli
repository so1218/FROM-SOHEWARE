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
};

// Light types
#define LIGHT_HALFLAMBERT 0
#define LIGHT_PHONG_SPECULAR 1
#define LIGHT_TOON 2
#define LIGHT_POINT 3
#define LIGHT_SPOT 4