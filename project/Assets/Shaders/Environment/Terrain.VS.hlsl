#include "Common/Object3D.hlsli"
#include "Common/ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<TerrainSettings> gTerrainSettings : register(b10); 

Texture2D<float> gTerrainHeightMap : register(t9);
StructuredBuffer<TerrainInstanceData> gTerrainInstances : register(t10);

SamplerState gSampler : register(s0);

struct TerrainVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

VertexShaderOutput main(TerrainVSInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    TerrainInstanceData inst = gTerrainInstances[instanceID];

    // ---------------------------------------------------------
    // ハイトマップサンプリングと頂点変位
    // ---------------------------------------------------------
    float2 globalUV = input.texcoord * inst.uvTransform.xy + inst.uvTransform.zw;
    
    // 0, 1 のテクスチャ値を -0.5, 0.5 へリマップし、基準面を中心とした高さを算出
    float rawHeight = gTerrainHeightMap.SampleLevel(gSampler, globalUV, 0).r;
    input.position.y = (rawHeight - 0.5f) * gTerrainSettings.maxHeight;

    // ---------------------------------------------------------
    // 座標変換とVelocity Bufferのプロパティ計算
    // ---------------------------------------------------------
    float4 worldPos = mul(input.position, inst.World);
    output.worldPosition = worldPos.xyz;
    
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    output.currentClipPos = output.position;
    output.prevClipPos = mul(worldPos, gFrameData.prevViewProj);
    
    output.texcoord = globalUV;
    
    // ---------------------------------------------------------
    // ハイトマップからの法線動的生成 
    // ---------------------------------------------------------
    float offset = gTerrainSettings.texelSize;
    
    // 差分計算においてベースの高さオフセットは相殺されるため、生の値をフェッチ
    float hL = gTerrainHeightMap.SampleLevel(gSampler, globalUV + float2(-offset, 0.0f), 0).r;
    float hR = gTerrainHeightMap.SampleLevel(gSampler, globalUV + float2(offset, 0.0f), 0).r;
    float hD = gTerrainHeightMap.SampleLevel(gSampler, globalUV + float2(0.0f, offset), 0).r;
    float hU = gTerrainHeightMap.SampleLevel(gSampler, globalUV + float2(0.0f, -offset), 0).r;
    
    // サンプリング後に1度だけ maxHeight を乗算
    float dx = (hL - hR) * gTerrainSettings.maxHeight;
    float dz = (hD - hU) * gTerrainSettings.maxHeight;
    
    float3 localNormal = normalize(float3(dx, 2.0f * gTerrainSettings.cellSize, dz));
    
    // インスタンスの非均等スケールを考慮し、法線は逆転置行列で変換
    output.normal = normalize(mul(localNormal, (float3x3) inst.WorldInverseTranspose));
    
    // 接ベクトルもローカル空間の固定値ではなく、ワールド行列で回転
    output.tangent = normalize(mul(float3(1.0f, 0.0f, 0.0f), (float3x3) inst.World));
    
    output.worldColor = inst.WorldColor;
    
    return output;
}