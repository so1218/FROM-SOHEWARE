#include "PSOManager.h"
#include "ShaderManager.h"
#include "RootSignatureManager.h"
#include <cassert>
#include <json.hpp>

void PSOManager::Initialize(
    ID3D12Device* device, 
    IDxcUtils* dxcUtils, 
    IDxcCompiler3* dxcCompiler, 
    IDxcIncludeHandler* includeHandler,
    RootSignatureManager* rootSignatureManager)
{
    device_ = device;
    rootSignatureManager_ = rootSignatureManager;

    CreateInputLayout();
    CompileShaders(dxcUtils, dxcCompiler, includeHandler);

    Create3DPSO();
    Create3DWireframePSO();
    CreateGridPSO();
    CreateLinePSO();
    CreateFullscreenPSO();
    CreateBloomBlurHorizontalPSO();
    CreateBrightnessExtractPSO();
    CreateBloomBlurVerticalPSO();
    CreateBloomCombinePSO();
    CreateDepthPSO();
}

void PSOManager::CreateInputLayout()
{
    // InputLayout
    inputElementDesc_[0].SemanticName = "POSITION";
    inputElementDesc_[0].SemanticIndex = 0;
    inputElementDesc_[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDesc_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDesc_[1].SemanticName = "TEXCOORD";
    inputElementDesc_[1].SemanticIndex = 0;
    inputElementDesc_[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDesc_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDesc_[2].SemanticName = "NORMAL";
    inputElementDesc_[2].SemanticIndex = 0;
    inputElementDesc_[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDesc_[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputLayoutDesc_.pInputElementDescs = inputElementDesc_;
    inputLayoutDesc_.NumElements = _countof(inputElementDesc_);

    // 粒子用 InputLayout
    inputElementDescsParticle_ = {
        // 頂点側
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },

        // インスタンス側（スロット1）
        { "WORLD",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "WORLD",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "WORLD",    2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "WORLD",    3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
    };

    // Layout に設定
    inputLayoutDescParticle_.pInputElementDescs = inputElementDescsParticle_.data();
    inputLayoutDescParticle_.NumElements = static_cast<UINT>(inputElementDescsParticle_.size());
}

void PSOManager::CompileShaders(IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler)
{
    // Shaderをコンパイルする
    vsBlob3D_ = ShaderManager::CompileShader(L"Resources/Shaders/Object3D.VS.hlsl",
        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(vsBlob3D_ != nullptr);

    psBlob3D_ = ShaderManager::CompileShader(L"Resources/Shaders/Object3D.PS.hlsl",
        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(psBlob3D_ != nullptr);

    // ライン用
    vsBlobLine_ = ShaderManager::CompileShader(L"Resources/Shaders/Line.VS.hlsl",
        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(vsBlobLine_ != nullptr);

    psBlobLine_ = ShaderManager::CompileShader(L"Resources/Shaders/Line.PS.hlsl",
        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(psBlobLine_ != nullptr);

    // パーティクル用
    vsBlobParticle_ = ShaderManager::CompileShader(L"Resources/Shaders/Particle.VS.hlsl",
        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(vsBlobParticle_ != nullptr);

    psBlobParticle_ = ShaderManager::CompileShader(L"Resources/Shaders/Particle.PS.hlsl",
        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(psBlobParticle_ != nullptr);

   
    psBlobExtract_ = ShaderManager::CompileShader(L"Resources/Shaders/BrightExtract.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(psBlobExtract_ != nullptr);


    psBlobBlurX_ = ShaderManager::CompileShader(L"Resources/Shaders/BlurX.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(psBlobBlurX_ != nullptr);


    psBlobBlurY_ = ShaderManager::CompileShader(L"Resources/Shaders/BlurY.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(psBlobBlurY_ != nullptr);

    psBlobBloomCombine_ = ShaderManager::CompileShader(L"Resources/Shaders/BloomCombine.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(psBlobBloomCombine_ != nullptr);

    vsBlobFullscreen_ = ShaderManager::CompileShader(
        L"Resources/Shaders/FullScreenQuad.VS.hlsl",
        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(vsBlobFullscreen_ != nullptr);

    psBlobFullscreen_ = ShaderManager::CompileShader(
        L"Resources/Shaders/FullScreenQuad.PS.hlsl",
        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(psBlobFullscreen_ != nullptr);

    vsBlobDepth_ = ShaderManager::CompileShader(
        L"Resources/Shaders/Depth.VS.hlsl",
        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(vsBlobDepth_ != nullptr);

    psBlobDepth_ = ShaderManager::CompileShader(
        L"Resources/Shaders/Depth.PS.hlsl",
        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
    assert(psBlobDepth_ != nullptr);
}

void PSOManager::Create3DPSO()
{
    // BlendStateの設定
    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // RasterizerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    // 裏面(時計回り)を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    //Depthの機能を有効化する
    depthStencilDesc.DepthEnable = true;
    // 書き込みをします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // PSOを生成する
    psoDesc3d_.pRootSignature = rootSignatureManager_->rootSignature3D_.Get();// RootSignature
    psoDesc3d_.InputLayout = inputLayoutDesc_;// InputLayout
    psoDesc3d_.VS = { vsBlob3D_->GetBufferPointer(),
    vsBlob3D_->GetBufferSize() };// VertexShader
    psoDesc3d_.PS = { psBlob3D_->GetBufferPointer(),
    psBlob3D_->GetBufferSize() };// PixelShader
    // DepthStencilの設定
    psoDesc3d_.DepthStencilState = depthStencilDesc;
    psoDesc3d_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc3d_.BlendState = blendDesc;// BlendState
    psoDesc3d_.RasterizerState = rasterizerDesc;// Rasterizer
    // 書き込むRTVの情報
    psoDesc3d_.NumRenderTargets = 1;
    psoDesc3d_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // 利用するとトポロジ(形状)のタイプ。三角形
    psoDesc3d_.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // どのように画面に打ち込むかの設定(気にしなくていい)
    psoDesc3d_.SampleDesc.Count = 1;
    psoDesc3d_.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc3d_,
        IID_PPV_ARGS(&pso3D_));
    assert(SUCCEEDED(hr));
}

void PSOManager::Create3DWireframePSO()
{
    // BlendStateの設定（通常と同じ）
    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // FillMode を WIREFRAME に変更
    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;  
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

    // DepthStencilState の設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // PSO 設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = rootSignatureManager_->rootSignature3D_.Get();
    desc.InputLayout = inputLayoutDesc_;
    desc.VS = { vsBlob3D_->GetBufferPointer(), vsBlob3D_->GetBufferSize() };
    desc.PS = { psBlob3D_->GetBufferPointer(), psBlob3D_->GetBufferSize() };
    desc.BlendState = blendDesc;
    desc.RasterizerState = rasterizerDesc;
    desc.DepthStencilState = depthStencilDesc;
    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.SampleDesc.Count = 1;
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    psoDesc3dWireFrame_ = desc;
    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc3dWireFrame_,
        IID_PPV_ARGS(&pso3DWireframe_));
    assert(SUCCEEDED(hr));
}
void PSOManager::CreateGridPSO()
{
    // BlendState（アルファブレンド有効）
    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // RasterizerState（裏面カリングあり or なしでも可）
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // DepthStencilState（書き込みなし = 透明部分でZバッファ更新を防ぐ）
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; 
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // PSO構築
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = rootSignatureManager_->rootSignature3D_.Get();
    desc.InputLayout = inputLayoutDesc_;                                
    desc.VS = { vsBlob3D_->GetBufferPointer(), vsBlob3D_->GetBufferSize() }; 
    desc.PS = { psBlob3D_->GetBufferPointer(), psBlob3D_->GetBufferSize() }; 
    desc.BlendState = blendDesc;
    desc.RasterizerState = rasterizerDesc;
    desc.DepthStencilState = depthStencilDesc;
    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.SampleDesc.Count = 1;
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // 実際にPSOを作成
    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoGrid_));
    assert(SUCCEEDED(hr));
}

void PSOManager::CreateLinePSO()
{
    // ライン用の設定
    psoDescLine_ = psoDesc3d_;  // 三角形用の設定をコピー
    psoDescLine_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;  // ライン用に変更

    psoDescLine_.RasterizerState.AntialiasedLineEnable = true;

    psoDescLine_.pRootSignature = rootSignatureManager_->rootSignatureLine_.Get();// RootSignature

    // ライン用シェーダに差し替え
    psoDescLine_.VS = { vsBlobLine_->GetBufferPointer(), vsBlobLine_->GetBufferSize() };
    psoDescLine_.PS = { psBlobLine_->GetBufferPointer(), psBlobLine_->GetBufferSize() };

    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDescLine_,
        IID_PPV_ARGS(&psoLine_));
    assert(SUCCEEDED(hr));
}

void PSOManager::CreateParticlePSO(BlendMode blendMode)
{
    // 三角形用をベースにコピー
    psoDescParticle_ = psoDesc3d_;

    // 粒子用のRootSignatureに差し替え
    psoDescParticle_.pRootSignature = rootSignatureManager_->rootSignatureParticles_.Get();

    // 粒子用シェーダに差し替え
    psoDescParticle_.VS = { vsBlobParticle_->GetBufferPointer(), vsBlobParticle_->GetBufferSize() };
    psoDescParticle_.PS = { psBlobParticle_->GetBufferPointer(), psBlobParticle_->GetBufferSize() };

    // 深度ステンシルの設定
    D3D12_DEPTH_STENCIL_DESC depthDesc = psoDescParticle_.DepthStencilState;
    depthDesc.DepthEnable = TRUE;

    switch (blendMode)
    {
    case kBlendModeNone:
        depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 深度書き込み有効
        break;

    case kBlendModeNormal:
    case kBlendModeAdd:
    case kBlendModeSubtract:
    case kBlendModeMultily:
    case kBlendModeScreen:
    case kBlendModeExclusion:
        depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 深度書き込み無効
        break;

    default:
        assert(false);
        break;
    }

    depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDescParticle_.DepthStencilState = depthDesc;

    psoDescParticle_.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // 入力レイアウト(インスタンシング)
    psoDescParticle_.InputLayout = inputLayoutDescParticle_;

    // ブレンド設定(引数のblendModeを使う)
    D3D12_RENDER_TARGET_BLEND_DESC blenddesc{};
    blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;

    switch (blendMode)
    {
    case kBlendModeNone:
        blenddesc.BlendEnable = FALSE;
        break;
    case kBlendModeNormal:
        blenddesc.BlendEnable = TRUE;
        blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
        blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        break;
    case kBlendModeAdd:
        blenddesc.BlendEnable = TRUE;
        blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
        blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blenddesc.DestBlend = D3D12_BLEND_ONE;
        break;
    case kBlendModeSubtract:
        blenddesc.BlendEnable = TRUE;
        blenddesc.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blenddesc.DestBlend = D3D12_BLEND_ONE;
        break;
    case kBlendModeMultily:
        blenddesc.BlendEnable = TRUE;
        blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
        blenddesc.SrcBlend = D3D12_BLEND_ZERO;
        blenddesc.DestBlend = D3D12_BLEND_SRC_COLOR;
        break;
    case kBlendModeScreen:
        blenddesc.BlendEnable = TRUE;
        blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
        blenddesc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        blenddesc.DestBlend = D3D12_BLEND_ONE;
        break;
    case kBlendModeExclusion:
        blenddesc.BlendEnable = TRUE;
        blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
        blenddesc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        blenddesc.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
        break;
    default:
        assert(false); // 不正なブレンドモード
        break;
    }

    D3D12_BLEND_DESC blendStateDesc = {};
    blendStateDesc.RenderTarget[0] = blenddesc;
    psoDescParticle_.BlendState = blendStateDesc;
}

void PSOManager::CreateAllParticlePipelines()
{
    for (int i = kBlendModeNone; i <= kBlendModeExclusion; ++i) {
        BlendMode mode = static_cast<BlendMode>(i);

        // パイプライン設定
        CreateParticlePSO(mode);

        // ブレンド設定の反映
        psoDescParticle_.BlendState.RenderTarget[0] = GetBlendDesc(mode);

        // PSO作成
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
        device_->CreateGraphicsPipelineState(&psoDescParticle_, IID_PPV_ARGS(&pso));
        psoParticles_[mode] = pso;
        pso->SetName(L"PSO_Particle");
    }
}

void PSOManager::CreateFullscreenPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    ZeroMemory(&desc, sizeof(desc));

    // Root Signature
    desc.pRootSignature = rootSignatureManager_->rootSignatureFullScreen_.Get();

    // VS / PS
    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
    desc.PS = { psBlobFullscreen_->GetBufferPointer(), psBlobFullscreen_->GetBufferSize() };

    // Input Layout（全画面三角形なので入力なし）
    desc.InputLayout = { nullptr, 0 };
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Blend State（不透明）
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.BlendState = blendDesc;

    // Rasterizer
    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.DepthClipEnable = TRUE;
    desc.RasterizerState = rasterizer;

    // Depth / Stencil（不要なので無効）
    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = FALSE;
    depthStencil.StencilEnable = FALSE;
    desc.DepthStencilState = depthStencil;
    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    // RTV
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.SampleDesc.Count = 1;

    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoFullscreen_));
    assert(SUCCEEDED(hr));
    psoFullscreen_->SetName(L"PSO_Fullscreen");
}

void PSOManager::CreateBrightnessExtractPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    ZeroMemory(&desc, sizeof(desc));

    // Root Signature
    desc.pRootSignature = rootSignatureManager_->rootSignaturePostProcess_.Get();

    // VS / PS
    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
    desc.PS = { psBlobExtract_->GetBufferPointer(), psBlobExtract_->GetBufferSize() };

    // Input Layout（全画面三角形なので入力なし）
    desc.InputLayout = { nullptr, 0 };
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Blend State（不透明）
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.BlendState = blendDesc;

    // Rasterizer
    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.DepthClipEnable = TRUE;
    desc.RasterizerState = rasterizer;

    // Depth / Stencil（不要なので無効）
    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = FALSE;
    depthStencil.StencilEnable = FALSE;
    desc.DepthStencilState = depthStencil;
    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    // RTV
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.SampleDesc.Count = 1;

    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoExtract_));
    assert(SUCCEEDED(hr));
    psoExtract_->SetName(L"PSO_BloomExtract");
}

void PSOManager::CreateBloomBlurVerticalPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    ZeroMemory(&desc, sizeof(desc));

    desc.pRootSignature = rootSignatureManager_->rootSignaturePostProcess_.Get();

    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
    desc.PS = { psBlobBlurY_->GetBufferPointer(), psBlobBlurY_->GetBufferSize() };

    desc.InputLayout = { nullptr, 0 };
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.BlendState = blendDesc;

    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.DepthClipEnable = TRUE;
    desc.RasterizerState = rasterizer;

    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = FALSE;
    depthStencil.StencilEnable = FALSE;
    desc.DepthStencilState = depthStencil;
    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.SampleDesc.Count = 1;

    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoBlurY_));
    assert(SUCCEEDED(hr));
    psoBlurY_->SetName(L"PSO_BloomBlurVertical");
}

void PSOManager::CreateBloomBlurHorizontalPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    ZeroMemory(&desc, sizeof(desc));

    desc.pRootSignature = rootSignatureManager_->rootSignaturePostProcess_.Get();

    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
    desc.PS = { psBlobBlurX_->GetBufferPointer(), psBlobBlurX_->GetBufferSize() };

    desc.InputLayout = { nullptr, 0 };
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.BlendState = blendDesc;

    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.DepthClipEnable = TRUE;
    desc.RasterizerState = rasterizer;

    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = FALSE;
    depthStencil.StencilEnable = FALSE;
    desc.DepthStencilState = depthStencil;
    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.SampleDesc.Count = 1;

    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoBlurX_));
    assert(SUCCEEDED(hr));
    psoBlurX_->SetName(L"PSO_BloomBlurHorizontal");
}

void PSOManager::CreateBloomCombinePSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    ZeroMemory(&desc, sizeof(desc));

    // Root Signature
    desc.pRootSignature = rootSignatureManager_->rootSignaturePostProcess_.Get();

    // VS / PS
    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
    desc.PS = { psBlobBloomCombine_->GetBufferPointer(), psBlobBloomCombine_->GetBufferSize() };

    // 入力レイアウト（全画面三角形なのでなし）
    desc.InputLayout = { nullptr, 0 };
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Blend State - 加算合成を設定
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC& rtBlendDesc = blendDesc.RenderTarget[0];
    rtBlendDesc.BlendEnable = TRUE;
    rtBlendDesc.LogicOpEnable = FALSE;
    rtBlendDesc.SrcBlend = D3D12_BLEND_ONE;
    rtBlendDesc.DestBlend = D3D12_BLEND_ONE;
    rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;

    // アルファを無視して RGB のみ書き込む
    rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_RED |
        D3D12_COLOR_WRITE_ENABLE_GREEN |
        D3D12_COLOR_WRITE_ENABLE_BLUE;

    desc.BlendState = blendDesc;

    // Rasterizer State
    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.DepthClipEnable = TRUE;
    desc.RasterizerState = rasterizer;

    // Depth Stencil State（不要なので無効）
    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = FALSE;
    depthStencil.StencilEnable = FALSE;
    desc.DepthStencilState = depthStencil;
    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    // Render Target
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    // Sample Mask
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.SampleDesc.Count = 1;

    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoBloomCombine_));
    assert(SUCCEEDED(hr));
    psoBloomCombine_->SetName(L"PSO_BloomCombine");
}

void  PSOManager::CreatePostEffectPassPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    ZeroMemory(&desc, sizeof(desc));

    // Root Signature
    desc.pRootSignature = rootSignatureManager_->rootSignatureFullScreen_.Get();

    // VS / PS
    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
    desc.PS = { psBlobFullscreen_->GetBufferPointer(), psBlobFullscreen_->GetBufferSize() };

    // Input Layout（全画面三角形なので入力なし）
    desc.InputLayout = { nullptr, 0 };
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Blend State（不透明）
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.BlendState = blendDesc;

    // Rasterizer
    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.DepthClipEnable = TRUE;
    desc.RasterizerState = rasterizer;

    // Depth / Stencil（不要なので無効）
    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = FALSE;
    depthStencil.StencilEnable = FALSE;
    desc.DepthStencilState = depthStencil;
    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    // RTV
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.SampleDesc.Count = 1;

    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoFullscreen_));
    assert(SUCCEEDED(hr));
    psoFullscreen_->SetName(L"PSO_PostEffectPass");
}

void PSOManager::CreateDepthPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    ZeroMemory(&desc, sizeof(desc));

    // Root Signature
    desc.pRootSignature = rootSignatureManager_->rootSignatureDepthExtract_.Get();

    // VS / PS
    // 深度パス用の頂点シェーダーとピクセルシェーダーのバイナリが必要です
    // ここでは仮に vertexShaderBlobDepth, pixelShaderBlobDepth とします
    desc.VS = { vsBlobDepth_->GetBufferPointer(), vsBlobDepth_->GetBufferSize() };
    desc.PS = { psBlobDepth_->GetBufferPointer(), psBlobDepth_->GetBufferSize() };

    D3D12_INPUT_LAYOUT_DESC inputLayoutDescDepth = {};
    inputLayoutDescDepth.pInputElementDescs = nullptr;
    inputLayoutDescDepth.NumElements = 0;
    desc.InputLayout = inputLayoutDescDepth;

    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // ブレンド設定は通常不要、色出力は1つだけ
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.BlendState = blendDesc;

    // ラスタライザー設定
    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_BACK;
    rasterizer.DepthClipEnable = TRUE;
    desc.RasterizerState = rasterizer;

    // デプスステンシルは通常無効にしてOK（深度をテクスチャとして書き出すため）
    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = FALSE;
    depthStencil.StencilEnable = FALSE;
    desc.DepthStencilState = depthStencil;
    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    // Render Target Format
    // 深度をレンダーターゲットとして出力する場合はfloat形式を使うことが多いです
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 例えばR32_FLOAT

    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.SampleDesc.Count = 1;

    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoDepth_));
    assert(SUCCEEDED(hr));
    psoDepth_->SetName(L"PSO_DepthPass");
}