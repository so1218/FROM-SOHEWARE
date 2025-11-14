#include "RootSignatureManager.h"
#include "RootSignatureBuilder.h"
#include "Logger.h"

void RootSignatureManager::Initialize(ID3D12Device* device)
{
    LOG_INFO("\n"
        "////////////////////////////////////////////////////////////\n"
        "//  RootSignatureManager Initialization Started.\n"
        "////////////////////////////////////////////////////////////");

    device_ = device;

    Create3dRootSignature();
    CreateSkinningRootSignature();
    CreateLineRootSignature();
    CreateParticleGraphicsRootSignature(); 
    CreatePostEffectPassRootSignature();
    CreateFullScreenRootSignature();
    CreateDepthExtractRootSignature();
    CreateSkyboxRootSignature();

    LOG_INFO("\n"
        "////////////////////////////////////////////////////////////\n"
        "//  RootSignatureManager Initialization Finished.\n"
        "////////////////////////////////////////////////////////////");
}

void RootSignatureManager::Create3dRootSignature()
{
    LOG_INFO("Creating 3D root signature...");

    RootSignatureBuilder builder;

    // [0] CBV (b0): ピクセルシェーダー用定数
    builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);

    // [1] CBV (b0): バーテックスシェーダー用定数
    builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);

    // [2] SRV(t0): メインテクスチャ
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);

    // [3] SRV(t1): 環境マップ
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);

    // [4-8] CBV(b1–b5): ピクセルシェーダー用追加定数
    builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
    builder.AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL);
    builder.AddCBV(3, D3D12_SHADER_VISIBILITY_PIXEL);
    builder.AddCBV(4, D3D12_SHADER_VISIBILITY_PIXEL);
    builder.AddCBV(5, D3D12_SHADER_VISIBILITY_PIXEL);

    // s0: 線形フィルタ + ラップ
    builder.AddStaticSampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_SHADER_VISIBILITY_PIXEL
    );

    rootSignature3D_ = builder.Build(
        device_,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
        "3D RootSignature"
    );

    if (rootSignature3D_)
    {
        LOG_INFO("Successfully created 3D root signature.");
    }
}

void RootSignatureManager::CreateSkinningRootSignature()
{
    LOG_INFO("Creating Skinning root signature...");

    RootSignatureBuilder builder;

    // [0] CBV(b0, VS): スキニング用定数
    builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);

    // [1] SRV(t0, VS): ボーン行列パレット
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);

    // [2] CBV(b0, PS): ピクセル用基本定数
    builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);

    // [3] SRV(t0, PS): メインテクスチャ
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);

    // [4] SRV(t1, PS): 環境マップ
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);

    // [5-9] CBV(b1–b5, PS): 各種ライト & カメラ定数
    builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL); // DirectionalLights
    builder.AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL); // Camera
    builder.AddCBV(3, D3D12_SHADER_VISIBILITY_PIXEL); // PointLights
    builder.AddCBV(4, D3D12_SHADER_VISIBILITY_PIXEL); // SpotLights
    builder.AddCBV(5, D3D12_SHADER_VISIBILITY_PIXEL); // AreaLights

    // s0: 線形フィルタ + ラップ
    builder.AddStaticSampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_SHADER_VISIBILITY_PIXEL
    );

    rootSignatureSkinning_ = builder.Build(
        device_,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
        "Skinning RootSignature"
    );

    if (rootSignatureSkinning_)
    {
        LOG_INFO("Successfully created Skinning root signature.");
    }
}

void RootSignatureManager::CreateLineRootSignature()
{
    LOG_INFO("Creating Line root signature...");

    RootSignatureBuilder builder;

    // [0] CBV (b0): 色などのライン用定数バッファ（VS/PS 共通）
    builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);

    // [1] CBV (b1): WVP 行列（Vertex Shader）
    builder.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX);

    // Sampler は使用しない

    rootSignatureLine_ = builder.Build(
        device_,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
        "Line RootSignature"
    );

    if (rootSignatureLine_)
    {
        LOG_INFO("Successfully created Line root signature.");
    }
}

void RootSignatureManager::CreateParticleGraphicsRootSignature()
{
    LOG_INFO("Creating Particle Graphics root signature...");

    RootSignatureBuilder builder;

    // [0] SRV(t0, VS): インスタンスデータ（ルートディスクリプタ）
    builder.AddSRV(0, D3D12_SHADER_VISIBILITY_VERTEX);

    // [1] CBV(b0, VS): パーティクル描画用定数
    builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);

    // [2] CBV(b1, PS): ピクセルシェーダー定数
    builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);

    // [3] SRV(t1, PS): パーティクルテクスチャ
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);

    // s0: 線形フィルタ + ラップ
    builder.AddStaticSampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_SHADER_VISIBILITY_PIXEL
    );

    rootSignatureParticles_ = builder.Build(
        device_,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
        "Particle Graphics RootSignature"
    );

    if (rootSignatureParticles_)
    {
        LOG_INFO("Successfully created Particle Graphics root signature.");
    }
}

void RootSignatureManager::CreatePostEffectPassRootSignature()
{
    LOG_INFO("Creating Post Effect Pass root signature...");

    RootSignatureBuilder builder;

    // [0] CBV(b0, PS): ポスト効果用定数
    builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);

    // [1] SRV(t0, PS): ブルームテクスチャ
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);

    // [2] SRV(t1, PS): 深度テクスチャ
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);

    // s0: 線形フィルタ + ラップ
    builder.AddStaticSampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_SHADER_VISIBILITY_PIXEL
    );

    rootSignatureFullScreen_ = builder.Build(
        device_,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
        "Post Effect Pass RootSignature"
    );

    if (rootSignatureFullScreen_)
        LOG_INFO("Successfully created Post Effect Pass root signature.");
}

void RootSignatureManager::CreateFullScreenRootSignature()
{
    LOG_INFO("Creating FullScreen root signature...");

    RootSignatureBuilder builder;

    // [0] CBV(b0, PS): フルスクリーン描画用定数
    builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);

    // [1] SRV(t0〜t7, PS): 最大8枚の入力テクスチャ
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 8, D3D12_SHADER_VISIBILITY_PIXEL);

    // s0: 線形フィルタ + クランプ
    builder.AddStaticSampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_SHADER_VISIBILITY_PIXEL
    );

    rootSignaturePostProcess_ = builder.Build(
        device_,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
        "FullScreen RootSignature"
    );

    if (rootSignaturePostProcess_)
        LOG_INFO("Successfully created FullScreen root signature.");
}

void RootSignatureManager::CreateDepthExtractRootSignature()
{
    LOG_INFO("Creating Depth Extract root signature...");

    RootSignatureBuilder builder;

    // [0] CBV(b0, VS): カメラ設定（VS）
    builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);

    // [1] CBV(b1, PS): カメラ設定（PS）
    builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);

    // [2] SRV(t0, PS): 深度テクスチャ
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);

    // s0: 線形フィルタ + ラップ
    builder.AddStaticSampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_SHADER_VISIBILITY_PIXEL
    );

    rootSignatureDepthExtract_ = builder.Build(
        device_,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
        "Depth Extract RootSignature"
    );

    if (rootSignatureDepthExtract_)
        LOG_INFO("Successfully created Depth Extract root signature.");
}

void RootSignatureManager::CreateSkyboxRootSignature()
{
    LOG_INFO("Creating Skybox root signature...");

    RootSignatureBuilder builder;

    // [0] CBV(b0, PS): マテリアルカラー
    builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);

    // [1] CBV(b1, VS): 変換行列
    builder.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX);

    // [2] SRV(t0, PS): キューブマップ / テクスチャ
    builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);

    // s0: 線形フィルタ + クランプ
    builder.AddStaticSampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_SHADER_VISIBILITY_PIXEL
    );

    rootSignatureSkybox_ = builder.Build(
        device_,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
        "Skybox RootSignature"
    );

    if (rootSignatureSkybox_)
        LOG_INFO("Successfully created Skybox root signature.");
}