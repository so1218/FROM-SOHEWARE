Texture2D<float4> gFireMap : register(t0);
RWTexture2D<float2> gReadbackMap : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint width, height;
    gReadbackMap.GetDimensions(width, height);
    if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
        return;

    uint sourceWidth, sourceHeight;
    gFireMap.GetDimensions(sourceWidth, sourceHeight);
    uint2 blockSize = uint2(sourceWidth / width, sourceHeight / height);
    uint2 blockStart = dispatchThreadId.xy * blockSize;
    
    float strongestFire = 0.0f;
    float burnProgressSum = 0.0f;
    for (uint offsetY = 0; offsetY < blockSize.y; ++offsetY)
    {
        for (uint offsetX = 0; offsetX < blockSize.x; ++offsetX)
        {
            float4 fireData = gFireMap.Load(int3(blockStart + uint2(offsetX, offsetY), 0));
            strongestFire = max(strongestFire, fireData.r);
            burnProgressSum += fireData.g;
        }
    }
    float texelCount = max((float) (blockSize.x * blockSize.y), 1.0f);
    gReadbackMap[dispatchThreadId.xy] = float2(strongestFire, burnProgressSum / texelCount);
}
