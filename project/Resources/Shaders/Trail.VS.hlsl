#include "Trail.hlsli"

// ランダム関数 (入力が同じなら常に同じ乱数を返す)
float hash11(float p)
{
    p = frac(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return frac(p);
}
float3 hash31(float p)
{
    float3 p3 = frac(float3(p, p, p) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xxy + p3.yzz) * p3.zyx) * 2.0 - 1.0;
}

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    // UVスクロール
    float time = gFrameData.gTime;
    float2 scroll = gTrailMaterial.scrollSpeed * time;
    output.texcoord = input.texcoord + scroll;
    output.texcoordRaw = input.texcoord;

    float3 pos = input.position.xyz;

    // -------------------------------------------------
    // ★ジッター計算
    // -------------------------------------------------
    if (gTrailMaterial.jitterStrength > 0.0)
    {
        // ★ポイント1: Phaseを足して「ずらし」に対応させる
        // これで floor の区切り位置が Phase 分だけ移動します
        float u = input.texcoord.x + gTrailMaterial.jitterPhase;
        
        float timeOffset = time * gTrailMaterial.jitterSpeed;
        float3 offset = float3(0, 0, 0);

        // -------------------------------------------------------
        // モード分岐
        // -------------------------------------------------------
        if (gTrailMaterial.jitterMode == 0)
        {
            // [Mode 0: Wave] 滑らかな波 (Sin波)
            offset.x = sin((u + timeOffset) * gTrailMaterial.jitterFrequency);
            offset.y = cos((u + timeOffset * 1.2) * gTrailMaterial.jitterFrequency);
            offset.z = sin((u + timeOffset * 0.8) * gTrailMaterial.jitterFrequency);
        }
        else if (gTrailMaterial.jitterMode == 1)
        {
            // [Mode 1: Lightning] 稲妻・ランダム (追加した処理)
            
            // ★ポイント2: floor計算にPhase込みのuを使う
            // これにより、エディタのPhaseスライダーでカクカクの位置が動く
            float uStep = floor(u * gTrailMaterial.jitterFrequency);
            float timeStep = floor(time * gTrailMaterial.jitterSpeed);

            float seed = uStep + timeStep * 13.0;
            
            // 3次元的にランダムにずらす
            offset = hash31(seed);
        }
        else
        {
            // [Mode 2: Digital Wave] デジタル・コマ送り波
            // Wave(Mode 0)の動きですが、Lightning(Mode 1)のようにカクカクさせます

            // ★ポイント: 連続値を floor で「階段状（整数）」にします
            // u * Frequency により、Frequency の密度で階段の段差が生まれます
            float uStep = floor(u * gTrailMaterial.jitterFrequency);
            
            // 時間も同様に階段状に進めます（これでコマ送り感がでます）
            float timeStep = floor(time * gTrailMaterial.jitterSpeed);

            // 階段状になった値を足し合わせて波を作ります
            // Mode 0 のようにさらに Frequency を掛ける必要はありません
            // (uStep の時点ですでに Frequency 分のステップになっているため)
            float stepInput = uStep + timeStep;

            offset.x = sin(stepInput);
            offset.y = cos(stepInput + stepInput * 0.2); // 少しずらす(Mode0の係数1.2に近い雰囲気で)
            offset.z = sin(stepInput - stepInput * 0.2); // 少しずらす
        }

        // -------------------------------------------------------
        // 座標への適用
        // -------------------------------------------------------
        // 以前のコードにあった `direction` (幅を広げる処理) は削除しました。
        // 稲妻や軌跡の移動としては、中心ごとずらす以下の計算が正解です。
        
        pos += offset * gTrailMaterial.jitterStrength;
    }
    
    output.position = mul(float4(pos, 1.0), gTransformationMatrix.WVP);
    output.color = input.color;

    return output;
}