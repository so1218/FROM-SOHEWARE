#include "Object3D.hlsli"
#include "ShaderConstants.hlsli"

ConstantBuffer<FrameData> gFrameData : register(b0);
ConstantBuffer<TransformationMatrix> gTransform : register(b6); // 地形専用の変換行列

struct TerrainVSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0; 
};

VertexShaderOutput main(TerrainVSInput input)
{
    VertexShaderOutput output;
    
    // 1. ワールド座標およびクリップ空間への変換
    float4 worldPos = mul(input.position, gTransform.World);
    output.worldPosition = worldPos.xyz;
    output.position = mul(worldPos, gFrameData.viewProjectionMatrix);
    output.currentClipPos = output.position;
    
    // 地形は静的オブジェクト（毎フレーム動かない）ため、過去のクリップ座標も現在と同じにする
    // これにより、速度バッファ（Velocity）の計算でブレが起きず、モーションブラーが綺麗にかかります
    output.prevClipPos = mul(worldPos, gFrameData.prevViewProj);

    // 2. テクスチャ座標のコピー
    output.texcoord = input.texcoord;
    
    // 3. 法線（Normal）の変換
    // スケールや回転に対応するため、WorldInverseTranspose の 3x3 部分を掛けます
    output.normal = normalize(mul(input.normal, (float3x3) gTransform.WorldInverseTranspose));
    
    // 4. タンジェント（Tangent）の変換
    // 接線ベクトルには通常の World 行列の 3x3 部分を掛けてワールド空間に変換します
    output.tangent = normalize(mul(input.tangent, (float3x3) gTransform.World));
    
    // 5. インスタンスカラー（マテリアルカラー等）の反映
    output.worldColor = gTransform.WorldColor;
    
    return output;
}