Texture3D<float4> gVelocityRead : register(t0);
Texture3D<float> gPressure : register(t1);
// u0: 書き込み先速度場（サイズ2に合わせるため、ダミーがu1にバインドされますがHLSL側はu0のみ使用でOK）
RWTexture3D<float4> gVelocityWrite : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocityWrite.GetDimensions(width, height, depth);
    if (any(DTid >= uint3(width, height, depth)))
        return;

    // 隣り合うボクセルの圧力を取得
    float pL = gPressure[max(DTid - uint3(1, 0, 0), 0)];
    float pR = gPressure[min(DTid + uint3(1, 0, 0), uint3(width - 1, height - 1, depth - 1))];
    float pB = gPressure[max(DTid - uint3(0, 1, 0), 0)];
    float pT = gPressure[min(DTid + uint3(0, 1, 0), uint3(width - 1, height - 1, depth - 1))];
    float pD = gPressure[max(DTid - uint3(0, 0, 1), 0)];
    float pU = gPressure[min(DTid + uint3(0, 0, 1), uint3(width - 1, height - 1, depth - 1))];

    // 圧力の差分から勾配（Gradient）を計算
    float3 gradient = 0.5f * float3(pR - pL, pT - pB, pU - pD);

    // 元の速度から勾配を引き算して、新しい速度バッファに書き込む
    float3 currentVel = gVelocityRead[DTid.xyz].xyz;
    gVelocityWrite[DTid.xyz] = float4(currentVel - gradient, 0.0f);
}