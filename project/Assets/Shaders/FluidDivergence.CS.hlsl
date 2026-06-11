Texture3D<float4> gVelocity : register(t0);
RWTexture3D<float> gDivergence : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocity.GetDimensions(width, height, depth);
    if (any(DTid >= uint3(width, height, depth)))
        return;

    // 隣り合うボクセルの速度を取得（境界では自分自身の速度でクランプするなどの処理が本来は必要）
    float3 vL = gVelocity[max(DTid - uint3(1, 0, 0), 0)].xyz;
    float3 vR = gVelocity[min(DTid + uint3(1, 0, 0), uint3(width - 1, height - 1, depth - 1))].xyz;
    float3 vB = gVelocity[max(DTid - uint3(0, 1, 0), 0)].xyz;
    float3 vT = gVelocity[min(DTid + uint3(0, 1, 0), uint3(width - 1, height - 1, depth - 1))].xyz;
    float3 vD = gVelocity[max(DTid - uint3(0, 0, 1), 0)].xyz;
    float3 vU = gVelocity[min(DTid + uint3(0, 0, 1), uint3(width - 1, height - 1, depth - 1))].xyz;

    // 速度の差分から発散（Divergence）を計算 (ハーフピクセルスケール)
    float divergence = 0.5f * ((vR.x - vL.x) + (vT.y - vB.y) + (vU.z - vD.z));
    
    gDivergence[DTid] = divergence;
}