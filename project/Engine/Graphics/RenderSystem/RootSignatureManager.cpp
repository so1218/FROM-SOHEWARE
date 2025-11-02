#include "RootSignatureManager.h"
#include "Logger.h"

void RootSignatureManager::Initialize(ID3D12Device* device)
{
    LOG_INFO("\n"
        "////////////////////////////////////////////////////////////\n"
        "//  RootSignatureManager Initialization Started.\n"
        "////////////////////////////////////////////////////////////");

    device_ = device;

    CreateLineRootSignature();
    Create3dRootSignature();
    CreateSkinningRootSignature();
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

void RootSignatureManager::CreateLineRootSignature()
{
    LOG_INFO("Creating Line root signature...");

    enum {
        kLineMaterialCbIndex = 0, // b0
        kLineWvpCbIndex = 1,      // b1
        kNumLineRootParameters
    };

    D3D12_ROOT_PARAMETER rootParameters[kNumLineRootParameters] = {};

    // RootParameter[0]: Material (色など) 用の定数バッファビュー (CBV)
    // シェーダーレジスタ: b0
    rootParameters[kLineMaterialCbIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kLineMaterialCbIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[kLineMaterialCbIndex].Descriptor.ShaderRegister = 0; // b0

    // RootParameter[1]: WVP行列用の定数バッファビュー (CBV)
    // シェーダーレジスタ: b1
    rootParameters[kLineWvpCbIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kLineWvpCbIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[kLineWvpCbIndex].Descriptor.ShaderRegister = 1; // b1

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = _countof(rootParameters);
    desc.pParameters = rootParameters;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob) 
        {
            const char* errorMsg = reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
            LOG_ERROR("Failed to serialize Line root signature: {}", errorMsg);
        }
        else 
        {
            LOG_ERROR("Failed to serialize Line root signature. HRESULT: {:#x}", hr);
        }
        assert(false); // デバッグビルドではここで停止
        return;
    }

    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignatureLine_));
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create Line root signature. HRESULT: {:#x}", hr);
        assert(false);
        return;
    }

    LOG_INFO("Successfully created Line root signature.");
}

void RootSignatureManager::Create3dRootSignature()
{
    LOG_INFO("Creating 3D root signature...");

    // DescriptorRangeの設定 (テクスチャSRV用)
    D3D12_DESCRIPTOR_RANGE srvDescriptorRange = {};
    srvDescriptorRange.BaseShaderRegister = 0; // t0
    srvDescriptorRange.NumDescriptors = 1;
    srvDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    enum {
        kPixelShaderCb0Index = 0, // b0 (PixelShader)
        kVertexShaderCb0Index = 1, // b0 (VertexShader)
        kTextureSrvTableIndex = 2, // t0 (PixelShader)
        kPixelShaderCb1Index = 3, // b1 (PixelShader)
        kPixelShaderCb2Index = 4, // b2 (PixelShader)
        kPixelShaderCb3Index = 5, // b3 (PixelShader)
        kPixelShaderCb4Index = 6, // b4 (PixelShader)
        kNumGraphicRootParameters
    };

    // RootParameter作成
    D3D12_ROOT_PARAMETER rootParameters[kNumGraphicRootParameters] = {};

    // RootParameter[0]: ピクセルシェーダー用CBV (b0)
    rootParameters[kPixelShaderCb0Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kPixelShaderCb0Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kPixelShaderCb0Index].Descriptor.ShaderRegister = 0; // b0

    // RootParameter[1]: バーテックスシェーダー用CBV (b0)
    rootParameters[kVertexShaderCb0Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kVertexShaderCb0Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[kVertexShaderCb0Index].Descriptor.ShaderRegister = 0; // b0

    // RootParameter[2]: テクスチャSRV用のDescriptorTable (t0)
    rootParameters[kTextureSrvTableIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[kTextureSrvTableIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kTextureSrvTableIndex].DescriptorTable.pDescriptorRanges = &srvDescriptorRange;
    rootParameters[kTextureSrvTableIndex].DescriptorTable.NumDescriptorRanges = 1;

    // RootParameter[3]: ピクセルシェーダー用CBV (b1)
    rootParameters[kPixelShaderCb1Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kPixelShaderCb1Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kPixelShaderCb1Index].Descriptor.ShaderRegister = 1; // b1

    // RootParameter[4]: ピクセルシェーダー用CBV (b2)
    rootParameters[kPixelShaderCb2Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kPixelShaderCb2Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kPixelShaderCb2Index].Descriptor.ShaderRegister = 2; // b2

    // RootParameter[5]: ピクセルシェーダー用CBV (b3)
    rootParameters[kPixelShaderCb3Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kPixelShaderCb3Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kPixelShaderCb3Index].Descriptor.ShaderRegister = 3; // b3

    // RootParameter[6]: ピクセルシェーダー用CBV (b4)
    rootParameters[kPixelShaderCb4Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kPixelShaderCb4Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kPixelShaderCb4Index].Descriptor.ShaderRegister = 4; // b4

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pParameters = rootParameters;

    // Samplerの設定 (テクスチャサンプリング用)
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0; // s0
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.NumStaticSamplers = 1;
    descriptionRootSignature.pStaticSamplers = &staticSampler;

    // シリアライズしてバイナリにする
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

    if (FAILED(hr)) 
    {
        if (errorBlob)
        {
            const char* errorMsg = reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
            LOG_ERROR("Failed to serialize 3D root signature: {}", errorMsg);
        }
        else 
        {
            LOG_ERROR("Failed to serialize 3D root signature. HRESULT: {:#x}", hr);
        }
        assert(false); 
        return;
    }

    // バイナリを元に生成
    hr = device_->CreateRootSignature(0,
        signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature3D_));
    if (FAILED(hr)) 
    {
        LOG_ERROR("Failed to create 3D root signature. HRESULT: {:#x}", hr);
        assert(false);
        return;
    }

    LOG_INFO("Successfully created 3D root signature.");
}

void RootSignatureManager::CreateSkinningRootSignature()
{
    LOG_INFO("Creating Skinning root signature...");

    // DescriptorRangeの設定 (テクスチャSRVをt0にバインド)
    D3D12_DESCRIPTOR_RANGE srvDescriptorRange = {};
    srvDescriptorRange.BaseShaderRegister = 0; // t0
    srvDescriptorRange.NumDescriptors = 1;    // 1つのSRV
    srvDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // VS用 SRVディスクリプタレンジ (gMatrixPalette用 t0)
    D3D12_DESCRIPTOR_RANGE srvMatrixPaletteDescriptorRange = {};
    srvMatrixPaletteDescriptorRange.BaseShaderRegister = 0; // t0 for Vertex Shader
    srvMatrixPaletteDescriptorRange.NumDescriptors = 1;
    srvMatrixPaletteDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvMatrixPaletteDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    enum {
        kVertexShaderCb0Index = 0, // b0 : 頂点シェーダー用定数バッファ
        kVertexShaderSrvT0Index = 1,
        kPixelShaderCb0Index = 2,  // b0 : ピクセルシェーダー用定数バッファ
        kTextureSrvTableIndex = 3, // t0 : テクスチャSRV
        kDirectionalLightsIndex = 4, // b1 : Directional Lights
        kCameraIndex = 5,            // b2 : Camera
        kPointLightsIndex = 6,       // b2 : Point Lights
        kSpotLightsIndex = 7,        // b3 : Spot Lights
        kNumRootParameters
    };

    D3D12_ROOT_PARAMETER rootParameters[kNumRootParameters] = {};

    // 頂点シェーダー用CBV (b0)
    rootParameters[kVertexShaderCb0Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kVertexShaderCb0Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[kVertexShaderCb0Index].Descriptor.ShaderRegister = 0; // b0

    // 頂点シェーダー用SRV (t0) for gMatrixPalette
    rootParameters[kVertexShaderSrvT0Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[kVertexShaderSrvT0Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[kVertexShaderSrvT0Index].DescriptorTable.pDescriptorRanges = &srvMatrixPaletteDescriptorRange;
    rootParameters[kVertexShaderSrvT0Index].DescriptorTable.NumDescriptorRanges = 1;

    // ピクセルシェーダー用CBV (b0)
    rootParameters[kPixelShaderCb0Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kPixelShaderCb0Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kPixelShaderCb0Index].Descriptor.ShaderRegister = 0; // b0

    // テクスチャのSRV設定 (t0)
    rootParameters[kTextureSrvTableIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[kTextureSrvTableIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kTextureSrvTableIndex].DescriptorTable.pDescriptorRanges = &srvDescriptorRange;
    rootParameters[kTextureSrvTableIndex].DescriptorTable.NumDescriptorRanges = 1;

    // Directional Lights (b1)
    rootParameters[kDirectionalLightsIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kDirectionalLightsIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kDirectionalLightsIndex].Descriptor.ShaderRegister = 1; // b1

    // Camera (b2)
    rootParameters[kCameraIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kCameraIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kCameraIndex].Descriptor.ShaderRegister = 2; // b2

    // Point Lights (b3)
    rootParameters[kPointLightsIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kPointLightsIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kPointLightsIndex].Descriptor.ShaderRegister = 3; // b3

    // Spot Lights (b4)
    rootParameters[kSpotLightsIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kSpotLightsIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kSpotLightsIndex].Descriptor.ShaderRegister = 4; // b4

    // サンプラの設定（3D用と同じ）
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0; // s0
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &staticSampler;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) 
    {
        if (errorBlob)
        {
            const char* errorMsg = reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
            LOG_ERROR("Failed to serialize Skinning root signature: {}", errorMsg);
        }
        else 
        {
            LOG_ERROR("Failed to serialize Skinning root signature. HRESULT: {:#x}", hr);
        }
        assert(false);
        return;
    }

    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignatureSkinning_));
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create Skinning root signature. HRESULT: {:#x}", hr);
        assert(false);
        return;
    }

    LOG_INFO("Successfully created Skinning root signature.");
}

// CreateParticleRootSignatureをCreateParticleGraphicsRootSignatureにリネームしました
void RootSignatureManager::CreateParticleGraphicsRootSignature()
{
    LOG_INFO("Creating Particle Graphics root signature...");

    // インスタンシングデータ用のSRVを設定するためのDescriptorRange
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 1;  // SRVのレジスタ番号
    descriptorRange[0].NumDescriptors = 1;     // 使用するSRVの数（今回は1つ）
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRV
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // RootParameter の設定（SRV用）
    D3D12_ROOT_PARAMETER rootParameters[4] = {};

    // インスタンスデータ（SRV）用
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    // 追加のCBVなど他のパラメータ（例）
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0;  // CBVのレジスタ番号

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].Descriptor.ShaderRegister = 1;  // 他のCBVのレジスタ番号

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
    rootParameters[3].DescriptorTable.pDescriptorRanges = descriptorRange;

    // Samplerの設定
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.MipLODBias = 0;
    staticSampler.MaxAnisotropy = 1;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    staticSampler.MinLOD = 0.0f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0; // s0 に対応
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // RootSignature の設定
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &staticSampler;

    // シリアライズ
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob) 
        {
            const char* errorMsg = reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
            LOG_ERROR("Failed to serialize Particle Graphics root signature: {}", errorMsg);
        }
        else 
        {
            LOG_ERROR("Failed to serialize Particle Graphics root signature. HRESULT: {:#x}", hr);
        }
        assert(false);
        return;
    }

    // RootSignature を作成
    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignatureParticles_));
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create Particle Graphics root signature. HRESULT: {:#x}", hr);
        assert(false);
        return;
    }

    LOG_INFO("Successfully created Particle Graphics root signature.");
}

void RootSignatureManager::CreatePostEffectPassRootSignature()
{
    LOG_INFO("Creating Post Effect Pass root signature...");

    // ----- Descriptor Range（Bloom用SRV: t0） -----
    D3D12_DESCRIPTOR_RANGE srvRangeBloom = {};
    srvRangeBloom.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRangeBloom.NumDescriptors = 1;
    srvRangeBloom.BaseShaderRegister = 0; // t0 (Bloomテクスチャ)
    srvRangeBloom.RegisterSpace = 0;
    srvRangeBloom.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // ----- Descriptor Range（深度テクスチャ用SRV: t1） -----
    D3D12_DESCRIPTOR_RANGE srvRangeDepth = {};
    srvRangeDepth.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRangeDepth.NumDescriptors = 1;
    srvRangeDepth.BaseShaderRegister = 1; // t1 (深度テクスチャ)
    srvRangeDepth.RegisterSpace = 0;
    srvRangeDepth.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // ----- Root Parameter: SRV Descriptor Table -----
    D3D12_ROOT_PARAMETER rootParameters[3] = {};

    // 1つ目のパラメータ（CBV: b0）
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0; // b0
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 2つ目のパラメータ（SRV Descriptor Table）
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &srvRangeBloom; // Bloom用

    // 3つ目のパラメータ（深度テクスチャ用SRV Descriptor Table）
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[2].DescriptorTable.pDescriptorRanges = &srvRangeDepth; // 深度用

    // ----- Static Sampler（s0） -----
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.MipLODBias = 0.0f;
    staticSampler.MaxAnisotropy = 1;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler.MinLOD = 0.0f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0; // s0
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // ----- Root Signature Description ----- 
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = _countof(rootParameters);
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &staticSampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // ----- シリアライズ ----- 
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob) 
        {
            const char* errorMsg = reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
            LOG_ERROR("Failed to serialize Post Effect Pass root signature: {}", errorMsg);
        }
        else 
        {
            LOG_ERROR("Failed to serialize Post Effect Pass root signature. HRESULT: {:#x}", hr);
        }
        assert(false);
        return;
    }

    // RootSignature 作成
    hr = device_->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignatureFullScreen_)
    );

    if (FAILED(hr)) 
    {
        LOG_ERROR("Failed to create Post Effect Pass root signature. HRESULT: {:#x}", hr);
        assert(false);
        return; 
    }

    LOG_INFO("Successfully created Post Effect Pass root signature.");
}

void RootSignatureManager::CreateFullScreenRootSignature()
{
    LOG_INFO("Creating FullScreen root signature...");

    // Descriptor Range（SRV: t0〜t1 の2つ）
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 8;               // SRVを2つ扱う
    srvRange.BaseShaderRegister = 0;           // t0からスタート
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Root Parameters (CBV + Descriptor Table for SRV)
    D3D12_ROOT_PARAMETER rootParameters[2] = {};

    // b0: Constant Buffer View (CBV)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0; // b0
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // t0〜t1: SRV Descriptor Table
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &srvRange;

    // Static Sampler (s0)
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.MipLODBias = 0.0f;
    staticSampler.MaxAnisotropy = 1;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler.MinLOD = 0.0f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0; // s0
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root Signature Description
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = _countof(rootParameters);
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &staticSampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // シリアライズと作成
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob) 
        {
            const char* errorMsg = reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
            LOG_ERROR("Failed to serialize FullScreen root signature: {}", errorMsg);
        }
        else 
        {
            LOG_ERROR("Failed to serialize FullScreen root signature. HRESULT: {:#x}", hr);
        }
        assert(false);
        return;
    }

    hr = device_->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignaturePostProcess_)
    );

    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create FullScreen root signature. HRESULT: {:#x}", hr);
        assert(false);
        return;
    }

    LOG_INFO("Successfully created FullScreen root signature.");
}

void RootSignatureManager::CreateDepthExtractRootSignature()
{
    LOG_INFO("Creating Depth Extract root signature...");

    // Descriptor Range（SRV: Depth 出力のための SRV）
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;               // 出力深度を格納する SRV
    srvRange.BaseShaderRegister = 0;           // t0 からスタート
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Root Parameters (CBV + SRV Descriptor Table)
    D3D12_ROOT_PARAMETER rootParameters[3] = {}; // b0, b1 と SRV

    // b0: Constant Buffer View (CBV) - CameraSettingsVS（頂点シェーダー用）
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0; // b0
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;  // 頂点シェーダー用

    // b1: Constant Buffer View (CBV) - CameraSettingsPS（ピクセルシェーダー用）
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 1; // b1
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // ピクセルシェーダー用

    // t0: SRV Descriptor Table (深度出力用)
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダー用
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[2].DescriptorTable.pDescriptorRanges = &srvRange;

    // Static Sampler (サンプリングが必要な場合)
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.MipLODBias = 0.0f;
    staticSampler.MaxAnisotropy = 1;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler.MinLOD = 0.0f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0; // s0
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root Signature Description
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = _countof(rootParameters);
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &staticSampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // シリアライズと作成
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob
    );

    if (FAILED(hr)) 
    {
        if (errorBlob)
        {
            const char* errorMsg = reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
            LOG_ERROR("Failed to serialize Depth Extract root signature: {}", errorMsg);
        }
        else 
        {
            LOG_ERROR("Failed to serialize Depth Extract root signature. HRESULT: {:#x}", hr);
        }
        assert(false);
        return;
    }

    hr = device_->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignatureDepthExtract_)
    );

    if (FAILED(hr))
    {
       
        LOG_ERROR("Failed to create Depth Extract root signature. HRESULT: {:#x}", hr);
        assert(false);
        return;
    }

    LOG_INFO("Successfully created Depth Extract root signature.");
}

void RootSignatureManager::CreateSkyboxRootSignature()
{
    LOG_INFO("Creating Skybox root signature...");

    // DescriptorRangeの設定 (テクスチャSRV用)
    D3D12_DESCRIPTOR_RANGE srvDescriptorRange = {};
    srvDescriptorRange.BaseShaderRegister = 0; // t0
    srvDescriptorRange.NumDescriptors = 1;
    srvDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    enum {
        kPixelShaderCb0Index = 0,   // b0 (PixelShader)
        kVertexShaderCb1Index = 1,  // b1 (VertexShader)
        kTextureSrvTableIndex = 2,  // t0 (PixelShader)
        kNumGraphicRootParameters
    };

    // RootParameter作成
    D3D12_ROOT_PARAMETER rootParameters[kNumGraphicRootParameters] = {};

    // RootParameter[0]: ピクセルシェーダー用CBV (b0) -> gMaterialColor
    rootParameters[kPixelShaderCb0Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kPixelShaderCb0Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kPixelShaderCb0Index].Descriptor.ShaderRegister = 0; // b0

    // RootParameter[1]: バーテックスシェーダー用CBV (b1) -> gTransformationMatrix
    rootParameters[kVertexShaderCb1Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[kVertexShaderCb1Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[kVertexShaderCb1Index].Descriptor.ShaderRegister = 1; // b1 

    // RootParameter[2]: テクスチャSRV用のDescriptorTable (t0) -> gTexture
    rootParameters[kTextureSrvTableIndex].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[kTextureSrvTableIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[kTextureSrvTableIndex].DescriptorTable.pDescriptorRanges = &srvDescriptorRange;
    rootParameters[kTextureSrvTableIndex].DescriptorTable.NumDescriptorRanges = 1;

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pParameters = rootParameters;

    // Samplerの設定 (s0)
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0; // s0
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.NumStaticSamplers = 1;
    descriptionRootSignature.pStaticSamplers = &staticSampler;

    // シリアライズ
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

    if (FAILED(hr))
    {
        LOG_ERROR("Failed to serialize Skybox root signature.");
        assert(false);
        return;
    }

    // バイナリを元に生成
    hr = device_->CreateRootSignature(0,
        signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignatureSkybox_));
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create Skybox root signature.");
        assert(false);
        return;
    }

    LOG_INFO("Successfully created Skybox root signature.");
}