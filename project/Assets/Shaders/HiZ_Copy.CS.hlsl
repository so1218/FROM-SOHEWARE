Texture2D<float> gInputDepth : register(t0);
RWTexture2D<float> gOutputDepth : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    gOutputDepth.GetDimensions(width, height);
    if (DTid.x >= width || DTid.y >= height)
        return;

    // 縮小せず、等倍でそのままコピー
    gOutputDepth[DTid.xy] = gInputDepth.Load(int3(DTid.xy, 0));
}