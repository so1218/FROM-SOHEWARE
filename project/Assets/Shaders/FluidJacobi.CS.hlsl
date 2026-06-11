Texture3D<float> gPressureRead : register(t0);
Texture3D<float> gDivergence : register(t1);
RWTexture3D<float> gPressureWrite : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gPressureWrite.GetDimensions(width, height, depth);
    if (any(DTid >= uint3(width, height, depth)))
        return;

    // 上下左右前後の圧力を取得
    float pL = gPressureRead[max(DTid - uint3(1, 0, 0), 0)];
    float pR = gPressureRead[min(DTid + uint3(1, 0, 0), uint3(width - 1, height - 1, depth - 1))];
    float pB = gPressureRead[max(DTid - uint3(0, 1, 0), 0)];
    float pT = gPressureRead[min(DTid + uint3(0, 1, 0), uint3(width - 1, height - 1, depth - 1))];
    float pD = gPressureRead[max(DTid - uint3(0, 0, 1), 0)];
    float pU = gPressureRead[min(DTid + uint3(0, 0, 1), uint3(width - 1, height - 1, depth - 1))];

    float div = gDivergence[DTid];

    // ヤコビ反復法による圧力の更新（3Dなので6方向の平均を取る）
    float newPressure = (pL + pR + pB + pT + pD + pU - div) / 6.0f;

    gPressureWrite[DTid] = newPressure;
}