Texture3D<float> gPressure : register(t0);
RWTexture3D<float4> gVelocity : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocity.GetDimensions(width, height, depth);
    if (any(DTid >= uint3(width, height, depth)))
        return;

    // 圧力の勾配を取得
    float pL = gPressure[max(DTid - uint3(1, 0, 0), 0)];
    float pR = gPressure[min(DTid + uint3(1, 0, 0), uint3(width - 1, height - 1, depth - 1))];
    float pB = gPressure[max(DTid - uint3(0, 1, 0), 0)];
    float pT = gPressure[min(DTid + uint3(0, 1, 0), uint3(width - 1, height - 1, depth - 1))];
    float pD = gPressure[max(DTid - uint3(0, 0, 1), 0)];
    float pU = gPressure[min(DTid + uint3(0, 0, 1), uint3(width - 1, height - 1, depth - 1))];

    float3 gradient = 0.5f * float3(pR - pL, pT - pB, pU - pD);

    // 現在の速度から圧力勾配を引くことで、圧縮されない自然な「渦を巻く」流れが完成する
    float3 currentVel = gVelocity[DTid].xyz;
    gVelocity[DTid] = float4(currentVel - gradient, 0.0f);
}