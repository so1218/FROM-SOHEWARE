#include "ShaderConstants.hlsli"

Texture3D gVelocityRead : register(t0);
Texture3D gDensityRead : register(t1);
Texture3D gUVWRead : register(t2);

RWTexture3D<float4> gVelocityWrite : register(u0);
RWTexture3D<float4> gDensityWrite : register(u1);
RWTexture3D<float4> gUVWWrite : register(u2);

ConstantBuffer<FluidSettings> gFluidSettings : register(b1);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height, depth;
    gVelocityWrite.GetDimensions(width, height, depth);
    if (any(DTid >= uint3(width, height, depth)))
        return;

    // 1. プレイヤーの移動と「逆方向」にデータをシフトするためのサンプリング座標を計算
    int3 srcCoord = (int3) DTid + gFluidSettings.voxelDelta;

    // 2. Toroidal Wrap (テクスチャサイズ内でループさせる算術)
    srcCoord.x = (srcCoord.x % (int) width + (int) width) % (int) width;
    srcCoord.y = (srcCoord.y % (int) height + (int) height) % (int) height;
    srcCoord.z = (srcCoord.z % (int) depth + (int) depth) % (int) depth;

    // 3. プレイヤーが移動したことによって「新しく箱の中に進入してきた領域」かどうかを判定する
    bool isNewArea = false;

    // X軸の進入判定
    if (gFluidSettings.voxelDelta.x > 0)
    {
        if (DTid.x >= width - uint(gFluidSettings.voxelDelta.x))
            isNewArea = true;
    }
    else if (gFluidSettings.voxelDelta.x < 0)
    {
        if (DTid.x < uint(-gFluidSettings.voxelDelta.x))
            isNewArea = true;
    }

    // Y軸の進入判定
    if (gFluidSettings.voxelDelta.y > 0)
    {
        if (DTid.y >= height - uint(gFluidSettings.voxelDelta.y))
            isNewArea = true;
    }
    else if (gFluidSettings.voxelDelta.y < 0)
    {
        if (DTid.y < uint(-gFluidSettings.voxelDelta.y))
            isNewArea = true;
    }

    // Z軸の進入判定
    if (gFluidSettings.voxelDelta.z > 0)
    {
        if (DTid.z >= depth - uint(gFluidSettings.voxelDelta.z))
            isNewArea = true;
    }
    else if (gFluidSettings.voxelDelta.z < 0)
    {
        if (DTid.z < uint(-gFluidSettings.voxelDelta.z))
            isNewArea = true;
    }

    // 4. 書き込み処理
    if (isNewArea)
    {
        // 新しく入ってきた領域は、古い回り込みデータを消すためにゼロクリア
        gVelocityWrite[DTid] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        gDensityWrite[DTid] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        
        // UVW座標だけは、そのボクセル自身の初期値を再計算して割り当てる
        float3 baseUVW = (float3(DTid) + 0.5f) / float3(width, height, depth);
        gUVWWrite[DTid] = float4(baseUVW, 0.0f);
    }
    else
    {
        // 移動していない既存の領域は、シフトした座標からデータをそのままコピー
        gVelocityWrite[DTid] = gVelocityRead[srcCoord];
        gDensityWrite[DTid] = gDensityRead[srcCoord];
        float3 uvwDelta = float3(gFluidSettings.voxelDelta) / float3(width, height, depth);
        gUVWWrite[DTid] = gUVWRead[srcCoord] - float4(uvwDelta, 0.0f);
    }
}