RWTexture3D<float4> gUVWWrite : register(u0);
RWTexture3D<float> gDensity : register(u1);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint w, h, d;
    gUVWWrite.GetDimensions(w, h, d);
    if (DTid.x >= w || DTid.y >= h || DTid.z >= d)
        return;
    gUVWWrite[DTid] = float4((float3(DTid) + 0.5f) / float3(w, h, d), 0.0f);
    
    gDensity[DTid] = 0.0f;
}