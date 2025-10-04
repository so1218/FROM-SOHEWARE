struct Particle
{
    float3 position;
    float3 velocity;
    float lifetime;
    float maxLifetime;
    float4 color;
    uint textureIndex;
};

struct InstanceData
{
    float4x4 world;
    float4 color;
    uint textureIndex;
    float rotationZ;
    float padding[2];
};
