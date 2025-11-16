#include "PSOManager.h"
#include "ShaderManager.h"
#include "RootSignatureManager.h"

#include <fstream>
#include <cassert>
#include <json.hpp>
//
//void PSOManager::Initialize(
//    ID3D12Device* device, 
//    IDxcUtils* dxcUtils, 
//    IDxcCompiler3* dxcCompiler, 
//    IDxcIncludeHandler* includeHandler,
//    RootSignatureManager* rootSignatureManager)
//{
//    device_ = device;
//    rootSignatureManager_ = rootSignatureManager;
//
//    CreateInputLayout();
//    CompileShaders(dxcUtils, dxcCompiler, includeHandler);
//
//    Create3DPSO();
//    Create3DWireframePSO();
//    CreateSkinningPSO();
//    CreateGridPSO();
//    CreateLinePSO();
//    CreateFullscreenPSO();
//    CreateBloomBlurHorizontalPSO();
//    CreateBrightnessExtractPSO();
//    CreateBloomBlurVerticalPSO();
//    CreateBloomCombinePSO();
//    CreateDepthPSO();
//    CreateSkyboxPSO();
//}
//
//void PSOManager::CreateInputLayout()
//{
//    // InputLayout
//    inputElementDesc_[0].SemanticName = "POSITION";
//    inputElementDesc_[0].SemanticIndex = 0;
//    inputElementDesc_[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
//    inputElementDesc_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
//    inputElementDesc_[1].SemanticName = "TEXCOORD";
//    inputElementDesc_[1].SemanticIndex = 0;
//    inputElementDesc_[1].Format = DXGI_FORMAT_R32G32_FLOAT;
//    inputElementDesc_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
//    inputElementDesc_[2].SemanticName = "NORMAL";
//    inputElementDesc_[2].SemanticIndex = 0;
//    inputElementDesc_[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
//    inputElementDesc_[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
//    inputElementDesc_[3].SemanticName = "WEIGHT";
//    inputElementDesc_[3].SemanticIndex = 0;
//    inputElementDesc_[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
//    inputElementDesc_[3].InputSlot = 1;
//    inputElementDesc_[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
//    inputElementDesc_[4].SemanticName = "INDEX";
//    inputElementDesc_[4].SemanticIndex = 0;
//    inputElementDesc_[4].Format = DXGI_FORMAT_R32G32B32A32_SINT; // int4
//    inputElementDesc_[4].InputSlot = 1;
//    inputElementDesc_[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
//
//    inputLayoutDesc_.pInputElementDescs = inputElementDesc_;
//    inputLayoutDesc_.NumElements = _countof(inputElementDesc_);
//
//    // 粒子用 InputLayout
//    inputElementDescsParticle_ = 
//    {
//        // 頂点側
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
//        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
//        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
//    };
//
//    // Layout に設定
//    inputLayoutDescParticle_.pInputElementDescs = inputElementDescsParticle_.data();
//    inputLayoutDescParticle_.NumElements = static_cast<UINT>(inputElementDescsParticle_.size());
//
//    inputElementDescSkybox_[0].SemanticName = "POSITION";
//    inputElementDescSkybox_[0].SemanticIndex = 0;
//    inputElementDescSkybox_[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // float3
//    inputElementDescSkybox_[0].InputSlot = 0;
//    inputElementDescSkybox_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
//    inputElementDescSkybox_[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
//    inputElementDescSkybox_[0].InstanceDataStepRate = 0;
//
//    inputLayoutDescSkybox_.pInputElementDescs = inputElementDescSkybox_;
//    inputLayoutDescSkybox_.NumElements = 1;
//}
//
//void PSOManager::CompileShaders(IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler)
//{
//    // Shaderをコンパイルする
//    vsBlob3D_ = ShaderManager::CompileShader(L"Resources/Shaders/Object3D.VS.hlsl",
//        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(vsBlob3D_ != nullptr);
//
//    psBlob3D_ = ShaderManager::CompileShader(L"Resources/Shaders/Object3D.PS.hlsl",
//        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(psBlob3D_ != nullptr);
//
//    // スキニング用
//    vsBlobSkinning_ = ShaderManager::CompileShader(L"Resources/Shaders/SkinningObject3D.VS.hlsl",
//        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(vsBlobSkinning_ != nullptr);
//
//    // ライン用
//    vsBlobLine_ = ShaderManager::CompileShader(L"Resources/Shaders/Line.VS.hlsl",
//        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(vsBlobLine_ != nullptr);
//
//    psBlobLine_ = ShaderManager::CompileShader(L"Resources/Shaders/Line.PS.hlsl",
//        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(psBlobLine_ != nullptr);
//
//    // パーティクル用
//    vsBlobParticle_ = ShaderManager::CompileShader(L"Resources/Shaders/Particle.VS.hlsl",
//        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(vsBlobParticle_ != nullptr);
//
//    psBlobParticle_ = ShaderManager::CompileShader(L"Resources/Shaders/Particle.PS.hlsl",
//        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(psBlobParticle_ != nullptr);
//
//   
//    psBlobExtract_ = ShaderManager::CompileShader(L"Resources/Shaders/BrightExtract.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(psBlobExtract_ != nullptr);
//
//
//    psBlobBlurX_ = ShaderManager::CompileShader(L"Resources/Shaders/BlurX.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(psBlobBlurX_ != nullptr);
//
//
//    psBlobBlurY_ = ShaderManager::CompileShader(L"Resources/Shaders/BlurY.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(psBlobBlurY_ != nullptr);
//
//    psBlobBloomCombine_ = ShaderManager::CompileShader(L"Resources/Shaders/BloomCombine.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(psBlobBloomCombine_ != nullptr);
//
//    vsBlobFullscreen_ = ShaderManager::CompileShader(
//        L"Resources/Shaders/FullScreenQuad.VS.hlsl",
//        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(vsBlobFullscreen_ != nullptr);
//
//    psBlobFullscreen_ = ShaderManager::CompileShader(
//        L"Resources/Shaders/FullScreenQuad.PS.hlsl",
//        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(psBlobFullscreen_ != nullptr);
//
//    vsBlobDepth_ = ShaderManager::CompileShader(
//        L"Resources/Shaders/Depth.VS.hlsl",
//        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(vsBlobDepth_ != nullptr);
//
//    psBlobDepth_ = ShaderManager::CompileShader(
//        L"Resources/Shaders/Depth.PS.hlsl",
//        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(psBlobDepth_ != nullptr);
//
//    vsBlobSkybox_ = ShaderManager::CompileShader(
//        L"Resources/Shaders/Skybox.VS.hlsl",
//        L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(vsBlobSkybox_ != nullptr);
//
//    psBlobSkybox_ = ShaderManager::CompileShader(
//        L"Resources/Shaders/Skybox.PS.hlsl",
//        L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
//    assert(psBlobSkybox_ != nullptr);
//}
//
//void PSOManager::Create3DPSO()
//{
//    // BlendStateの設定
//    D3D12_BLEND_DESC blendDesc{};
//    blendDesc.RenderTarget[0].BlendEnable = TRUE;
//    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
//    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
//    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
//    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
//    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
//    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//
//    // RasterizerStateの設定
//    D3D12_RASTERIZER_DESC rasterizerDesc{};
//    // 裏面(時計回り)を表示しない
//    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
//    // 三角形の中を塗りつぶす
//    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
//
//    // DepthStencilStateの設定
//    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
//    //Depthの機能を有効化する
//    depthStencilDesc.DepthEnable = true;
//    // 書き込みをします
//    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
//    // 比較関数はLessEqual。つまり、近ければ描画される
//    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
//
//    // PSOを生成する
//    psoDesc3d_.pRootSignature = rootSignatureManager_->Get3DRootSignature();// RootSignature
//    psoDesc3d_.InputLayout = inputLayoutDesc_;// InputLayout
//    psoDesc3d_.VS = { vsBlob3D_->GetBufferPointer(),
//    vsBlob3D_->GetBufferSize() };// VertexShader
//    psoDesc3d_.PS = { psBlob3D_->GetBufferPointer(),
//    psBlob3D_->GetBufferSize() };// PixelShader
//    // DepthStencilの設定
//    psoDesc3d_.DepthStencilState = depthStencilDesc;
//    psoDesc3d_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
//    psoDesc3d_.BlendState = blendDesc;// BlendState
//    psoDesc3d_.RasterizerState = rasterizerDesc;// Rasterizer
//    // 書き込むRTVの情報
//    psoDesc3d_.NumRenderTargets = 1;
//    psoDesc3d_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//    // 利用するとトポロジ(形状)のタイプ。三角形
//    psoDesc3d_.PrimitiveTopologyType =
//        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//    // どのように画面に打ち込むかの設定(気にしなくていい)
//    psoDesc3d_.SampleDesc.Count = 1;
//    psoDesc3d_.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc3d_,
//        IID_PPV_ARGS(&pso3D_));
//    assert(SUCCEEDED(hr));
//}
//
//void PSOManager::Create3DWireframePSO()
//{
//    // BlendStateの設定（通常と同じ）
//    D3D12_BLEND_DESC blendDesc{};
//    blendDesc.RenderTarget[0].BlendEnable = TRUE;
//    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
//    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
//    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
//    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
//    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
//    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//
//    // FillMode を WIREFRAME に変更
//    D3D12_RASTERIZER_DESC rasterizerDesc = {};
//    rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;  
//    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
//
//    // DepthStencilState の設定
//    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
//    depthStencilDesc.DepthEnable = true;
//    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
//    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
//
//    // PSO 設定
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
//    desc.pRootSignature = rootSignatureManager_->Get3DRootSignature();
//    desc.InputLayout = inputLayoutDesc_;
//    desc.VS = { vsBlob3D_->GetBufferPointer(), vsBlob3D_->GetBufferSize() };
//    desc.PS = { psBlob3D_->GetBufferPointer(), psBlob3D_->GetBufferSize() };
//    desc.BlendState = blendDesc;
//    desc.RasterizerState = rasterizerDesc;
//    desc.DepthStencilState = depthStencilDesc;
//    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
//    desc.NumRenderTargets = 1;
//    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//    desc.SampleDesc.Count = 1;
//    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//
//    psoDesc3dWireFrame_ = desc;
//    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc3dWireFrame_,
//        IID_PPV_ARGS(&pso3DWireframe_));
//    assert(SUCCEEDED(hr));
//}
//
//void PSOManager::CreateSkinningPSO()
//{
//    // BlendStateの設定
//    D3D12_BLEND_DESC blendDesc{};
//    blendDesc.RenderTarget[0].BlendEnable = TRUE;
//    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
//    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
//    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
//    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
//    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
//    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//
//    // RasterizerStateの設定
//    D3D12_RASTERIZER_DESC rasterizerDesc{};
//    // 裏面(時計回り)を表示しない
//    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
//    // 三角形の中を塗りつぶす
//    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
//
//    // DepthStencilStateの設定
//    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
//    //Depthの機能を有効化する
//    depthStencilDesc.DepthEnable = true;
//    // 書き込みをします
//    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
//    // 比較関数はLessEqual。つまり、近ければ描画される
//    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
//
//    // PSOを生成する
//    psoDescSkinning_.pRootSignature = rootSignatureManager_->GetSkinningRootSignature();// RootSignature
//    psoDescSkinning_.InputLayout = inputLayoutDesc_;// InputLayout
//    psoDescSkinning_.VS = { vsBlobSkinning_->GetBufferPointer(),
//    vsBlobSkinning_->GetBufferSize() };// VertexShader
//    psoDescSkinning_.PS = { psBlob3D_->GetBufferPointer(),
//    psBlob3D_->GetBufferSize() };// PixelShader
//    // DepthStencilの設定
//    psoDescSkinning_.DepthStencilState = depthStencilDesc;
//    psoDescSkinning_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
//    psoDescSkinning_.BlendState = blendDesc;// BlendState
//    psoDescSkinning_.RasterizerState = rasterizerDesc;// Rasterizer
//    // 書き込むRTVの情報
//    psoDescSkinning_.NumRenderTargets = 1;
//    psoDescSkinning_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//    // 利用するとトポロジ(形状)のタイプ。三角形
//    psoDescSkinning_.PrimitiveTopologyType =
//        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//    // どのように画面に打ち込むかの設定(気にしなくていい)
//    psoDescSkinning_.SampleDesc.Count = 1;
//    psoDescSkinning_.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDescSkinning_,
//        IID_PPV_ARGS(&psoSkinning_));
//    assert(SUCCEEDED(hr));
//}
//
//
//void PSOManager::CreateGridPSO()
//{
//    // BlendState（アルファブレンド有効）
//    D3D12_BLEND_DESC blendDesc{};
//    blendDesc.RenderTarget[0].BlendEnable = TRUE;
//    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
//    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
//    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
//    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
//    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
//    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//
//    // RasterizerState（裏面カリングあり or なしでも可）
//    D3D12_RASTERIZER_DESC rasterizerDesc{};
//    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
//    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
//
//    // DepthStencilState（書き込みなし = 透明部分でZバッファ更新を防ぐ）
//    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
//    depthStencilDesc.DepthEnable = TRUE;
//    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; 
//    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
//
//    // PSO構築
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
//    desc.pRootSignature = rootSignatureManager_->Get3DRootSignature();
//    desc.InputLayout = inputLayoutDesc_;                                
//    desc.VS = { vsBlob3D_->GetBufferPointer(), vsBlob3D_->GetBufferSize() }; 
//    desc.PS = { psBlob3D_->GetBufferPointer(), psBlob3D_->GetBufferSize() }; 
//    desc.BlendState = blendDesc;
//    desc.RasterizerState = rasterizerDesc;
//    desc.DepthStencilState = depthStencilDesc;
//    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
//    desc.NumRenderTargets = 1;
//    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//    desc.SampleDesc.Count = 1;
//    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//
//    // 実際にPSOを作成
//    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoGrid_));
//    assert(SUCCEEDED(hr));
//}
//
//void PSOManager::CreateLinePSO()
//{
//    // ライン用の設定
//    psoDescLine_ = psoDesc3d_;  // 三角形用の設定をコピー
//    psoDescLine_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;  // ライン用に変更
//
//    psoDescLine_.RasterizerState.AntialiasedLineEnable = true;
//
//    psoDescLine_.pRootSignature = rootSignatureManager_->GetLineRootSignature();// RootSignature
//
//    // ライン用シェーダに差し替え
//    psoDescLine_.VS = { vsBlobLine_->GetBufferPointer(), vsBlobLine_->GetBufferSize() };
//    psoDescLine_.PS = { psBlobLine_->GetBufferPointer(), psBlobLine_->GetBufferSize() };
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDescLine_,
//        IID_PPV_ARGS(&psoLine_));
//    assert(SUCCEEDED(hr));
//}
//
//void PSOManager::CreateParticlePSO(BlendMode blendMode)
//{
//    // 三角形用をベースにコピー
//    psoDescParticle_ = psoDesc3d_;
//
//    // 粒子用のRootSignatureに差し替え
//    psoDescParticle_.pRootSignature = rootSignatureManager_->GetParticleRootSignature();
//
//    // 粒子用シェーダに差し替え
//    psoDescParticle_.VS = { vsBlobParticle_->GetBufferPointer(), vsBlobParticle_->GetBufferSize() };
//    psoDescParticle_.PS = { psBlobParticle_->GetBufferPointer(), psBlobParticle_->GetBufferSize() };
//
//    // 深度ステンシルの設定
//    D3D12_DEPTH_STENCIL_DESC depthDesc = psoDescParticle_.DepthStencilState;
//    depthDesc.DepthEnable = TRUE;
//
//    switch (blendMode)
//    {
//    case kBlendModeNone:
//        depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 深度書き込み有効
//        break;
//
//    case kBlendModeNormal:
//    case kBlendModeAdd:
//    case kBlendModeSubtract:
//    case kBlendModeMultily:
//    case kBlendModeScreen:
//    case kBlendModeExclusion:
//        depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 深度書き込み無効
//        break;
//
//    default:
//        assert(false);
//        break;
//    }
//
//    depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
//    psoDescParticle_.DepthStencilState = depthDesc;
//
//    psoDescParticle_.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
//
//    // 入力レイアウト(インスタンシング)
//    psoDescParticle_.InputLayout = inputLayoutDescParticle_;
//
//    // ブレンド設定(引数のblendModeを使う)
//    D3D12_RENDER_TARGET_BLEND_DESC blenddesc{};
//    blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//    blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
//    blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;
//    blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;
//
//    switch (blendMode)
//    {
//    case kBlendModeNone:
//        blenddesc.BlendEnable = FALSE;
//        break;
//    case kBlendModeNormal:
//        blenddesc.BlendEnable = TRUE;
//        blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
//        blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
//        blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
//        break;
//    case kBlendModeAdd:
//        blenddesc.BlendEnable = TRUE;
//        blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
//        blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
//        blenddesc.DestBlend = D3D12_BLEND_ONE;
//        break;
//    case kBlendModeSubtract:
//        blenddesc.BlendEnable = TRUE;
//        blenddesc.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
//        blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
//        blenddesc.DestBlend = D3D12_BLEND_ONE;
//        break;
//    case kBlendModeMultily:
//        blenddesc.BlendEnable = TRUE;
//        blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
//        blenddesc.SrcBlend = D3D12_BLEND_ZERO;
//        blenddesc.DestBlend = D3D12_BLEND_SRC_COLOR;
//        break;
//    case kBlendModeScreen:
//        blenddesc.BlendEnable = TRUE;
//        blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
//        blenddesc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
//        blenddesc.DestBlend = D3D12_BLEND_ONE;
//        break;
//    case kBlendModeExclusion:
//        blenddesc.BlendEnable = TRUE;
//        blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
//        blenddesc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
//        blenddesc.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
//        break;
//    default:
//        assert(false); // 不正なブレンドモード
//        break;
//    }
//
//    D3D12_BLEND_DESC blendStateDesc = {};
//    blendStateDesc.RenderTarget[0] = blenddesc;
//    psoDescParticle_.BlendState = blendStateDesc;
//}
//
//void PSOManager::CreateAllParticlePipelines()
//{
//    for (int i = kBlendModeNone; i <= kBlendModeExclusion; ++i) {
//        BlendMode mode = static_cast<BlendMode>(i);
//
//        // パイプライン設定
//        CreateParticlePSO(mode);
//
//        // ブレンド設定の反映
//        psoDescParticle_.BlendState.RenderTarget[0] = GetBlendDesc(mode);
//
//        // PSO作成
//        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
//        device_->CreateGraphicsPipelineState(&psoDescParticle_, IID_PPV_ARGS(&pso));
//        psoParticles_[mode] = pso;
//        pso->SetName(L"PSO_Particle");
//    }
//}
//
//void PSOManager::CreateFullscreenPSO()
//{
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
//    ZeroMemory(&desc, sizeof(desc));
//
//    // Root Signature
//    desc.pRootSignature = rootSignatureManager_->GetFullScreenRootSignature();
//
//    // VS / PS
//    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
//    desc.PS = { psBlobFullscreen_->GetBufferPointer(), psBlobFullscreen_->GetBufferSize() };
//
//    // Input Layout（全画面三角形なので入力なし）
//    desc.InputLayout = { nullptr, 0 };
//    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//
//    // Blend State（不透明）
//    D3D12_BLEND_DESC blendDesc = {};
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//    desc.BlendState = blendDesc;
//
//    // Rasterizer
//    D3D12_RASTERIZER_DESC rasterizer = {};
//    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
//    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
//    rasterizer.DepthClipEnable = TRUE;
//    desc.RasterizerState = rasterizer;
//
//    // Depth / Stencil（不要なので無効）
//    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
//    depthStencil.DepthEnable = FALSE;
//    depthStencil.StencilEnable = FALSE;
//    desc.DepthStencilState = depthStencil;
//    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
//
//    // RTV
//    desc.NumRenderTargets = 1;
//    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//
//    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//    desc.SampleDesc.Count = 1;
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoFullscreen_));
//    assert(SUCCEEDED(hr));
//    psoFullscreen_->SetName(L"PSO_Fullscreen");
//}
//
//void PSOManager::CreateBrightnessExtractPSO()
//{
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
//    ZeroMemory(&desc, sizeof(desc));
//
//    // Root Signature
//    desc.pRootSignature = rootSignatureManager_->GetPostProcessRootSignature();
//
//    // VS / PS
//    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
//    desc.PS = { psBlobExtract_->GetBufferPointer(), psBlobExtract_->GetBufferSize() };
//
//    // Input Layout（全画面三角形なので入力なし）
//    desc.InputLayout = { nullptr, 0 };
//    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//
//    // Blend State（不透明）
//    D3D12_BLEND_DESC blendDesc = {};
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//    desc.BlendState = blendDesc;
//
//    // Rasterizer
//    D3D12_RASTERIZER_DESC rasterizer = {};
//    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
//    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
//    rasterizer.DepthClipEnable = TRUE;
//    desc.RasterizerState = rasterizer;
//
//    // Depth / Stencil（不要なので無効）
//    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
//    depthStencil.DepthEnable = FALSE;
//    depthStencil.StencilEnable = FALSE;
//    desc.DepthStencilState = depthStencil;
//    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
//
//    // RTV
//    desc.NumRenderTargets = 1;
//    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//
//    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//    desc.SampleDesc.Count = 1;
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoExtract_));
//    assert(SUCCEEDED(hr));
//    psoExtract_->SetName(L"PSO_BloomExtract");
//}
//
//void PSOManager::CreateBloomBlurVerticalPSO()
//{
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
//    ZeroMemory(&desc, sizeof(desc));
//
//    desc.pRootSignature = rootSignatureManager_->GetPostProcessRootSignature();
//
//    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
//    desc.PS = { psBlobBlurY_->GetBufferPointer(), psBlobBlurY_->GetBufferSize() };
//
//    desc.InputLayout = { nullptr, 0 };
//    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//
//    D3D12_BLEND_DESC blendDesc = {};
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//    desc.BlendState = blendDesc;
//
//    D3D12_RASTERIZER_DESC rasterizer = {};
//    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
//    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
//    rasterizer.DepthClipEnable = TRUE;
//    desc.RasterizerState = rasterizer;
//
//    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
//    depthStencil.DepthEnable = FALSE;
//    depthStencil.StencilEnable = FALSE;
//    desc.DepthStencilState = depthStencil;
//    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
//
//    desc.NumRenderTargets = 1;
//    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//
//    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//    desc.SampleDesc.Count = 1;
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoBlurY_));
//    assert(SUCCEEDED(hr));
//    psoBlurY_->SetName(L"PSO_BloomBlurVertical");
//}
//
//void PSOManager::CreateBloomBlurHorizontalPSO()
//{
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
//    ZeroMemory(&desc, sizeof(desc));
//
//    desc.pRootSignature = rootSignatureManager_->GetPostProcessRootSignature();
//
//    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
//    desc.PS = { psBlobBlurX_->GetBufferPointer(), psBlobBlurX_->GetBufferSize() };
//
//    desc.InputLayout = { nullptr, 0 };
//    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//
//    D3D12_BLEND_DESC blendDesc = {};
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//    desc.BlendState = blendDesc;
//
//    D3D12_RASTERIZER_DESC rasterizer = {};
//    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
//    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
//    rasterizer.DepthClipEnable = TRUE;
//    desc.RasterizerState = rasterizer;
//
//    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
//    depthStencil.DepthEnable = FALSE;
//    depthStencil.StencilEnable = FALSE;
//    desc.DepthStencilState = depthStencil;
//    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
//
//    desc.NumRenderTargets = 1;
//    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//
//    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//    desc.SampleDesc.Count = 1;
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoBlurX_));
//    assert(SUCCEEDED(hr));
//    psoBlurX_->SetName(L"PSO_BloomBlurHorizontal");
//}
//
//void PSOManager::CreateBloomCombinePSO()
//{
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
//    ZeroMemory(&desc, sizeof(desc));
//
//    // Root Signature
//    desc.pRootSignature = rootSignatureManager_->GetPostProcessRootSignature();
//
//    // VS / PS
//    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
//    desc.PS = { psBlobBloomCombine_->GetBufferPointer(), psBlobBloomCombine_->GetBufferSize() };
//
//    // 入力レイアウト（全画面三角形なのでなし）
//    desc.InputLayout = { nullptr, 0 };
//    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//
//    // Blend State - 加算合成を設定
//    D3D12_BLEND_DESC blendDesc = {};
//    blendDesc.AlphaToCoverageEnable = FALSE;
//    blendDesc.IndependentBlendEnable = FALSE;
//
//    D3D12_RENDER_TARGET_BLEND_DESC& rtBlendDesc = blendDesc.RenderTarget[0];
//    rtBlendDesc.BlendEnable = TRUE;
//    rtBlendDesc.LogicOpEnable = FALSE;
//    rtBlendDesc.SrcBlend = D3D12_BLEND_ONE;
//    rtBlendDesc.DestBlend = D3D12_BLEND_ONE;
//    rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
//    rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
//    rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
//    rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
//
//    // アルファを無視して RGB のみ書き込む
//    rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_RED |
//        D3D12_COLOR_WRITE_ENABLE_GREEN |
//        D3D12_COLOR_WRITE_ENABLE_BLUE;
//
//    desc.BlendState = blendDesc;
//
//    // Rasterizer State
//    D3D12_RASTERIZER_DESC rasterizer = {};
//    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
//    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
//    rasterizer.DepthClipEnable = TRUE;
//    desc.RasterizerState = rasterizer;
//
//    // Depth Stencil State（不要なので無効）
//    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
//    depthStencil.DepthEnable = FALSE;
//    depthStencil.StencilEnable = FALSE;
//    desc.DepthStencilState = depthStencil;
//    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
//
//    // Render Target
//    desc.NumRenderTargets = 1;
//    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//
//    // Sample Mask
//    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//    desc.SampleDesc.Count = 1;
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoBloomCombine_));
//    assert(SUCCEEDED(hr));
//    psoBloomCombine_->SetName(L"PSO_BloomCombine");
//}
//
//void  PSOManager::CreatePostEffectPassPSO()
//{
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
//    ZeroMemory(&desc, sizeof(desc));
//
//    // Root Signature
//    desc.pRootSignature = rootSignatureManager_->GetFullScreenRootSignature();
//
//    // VS / PS
//    desc.VS = { vsBlobFullscreen_->GetBufferPointer(), vsBlobFullscreen_->GetBufferSize() };
//    desc.PS = { psBlobFullscreen_->GetBufferPointer(), psBlobFullscreen_->GetBufferSize() };
//
//    // Input Layout（全画面三角形なので入力なし）
//    desc.InputLayout = { nullptr, 0 };
//    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//
//    // Blend State（不透明）
//    D3D12_BLEND_DESC blendDesc = {};
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//    desc.BlendState = blendDesc;
//
//    // Rasterizer
//    D3D12_RASTERIZER_DESC rasterizer = {};
//    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
//    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
//    rasterizer.DepthClipEnable = TRUE;
//    desc.RasterizerState = rasterizer;
//
//    // Depth / Stencil（不要なので無効）
//    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
//    depthStencil.DepthEnable = FALSE;
//    depthStencil.StencilEnable = FALSE;
//    desc.DepthStencilState = depthStencil;
//    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
//
//    // RTV
//    desc.NumRenderTargets = 1;
//    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//
//    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//    desc.SampleDesc.Count = 1;
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoFullscreen_));
//    assert(SUCCEEDED(hr));
//    psoFullscreen_->SetName(L"PSO_PostEffectPass");
//}
//
//void PSOManager::CreateDepthPSO()
//{
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
//    ZeroMemory(&desc, sizeof(desc));
//
//    // Root Signature
//    desc.pRootSignature = rootSignatureManager_->GetDepthExtractRootSignature();
//
//    // VS / PS
//    // 深度パス用の頂点シェーダーとピクセルシェーダーのバイナリが必要です
//    // ここでは仮に vertexShaderBlobDepth, pixelShaderBlobDepth とします
//    desc.VS = { vsBlobDepth_->GetBufferPointer(), vsBlobDepth_->GetBufferSize() };
//    desc.PS = { psBlobDepth_->GetBufferPointer(), psBlobDepth_->GetBufferSize() };
//
//    D3D12_INPUT_LAYOUT_DESC inputLayoutDescDepth = {};
//    inputLayoutDescDepth.pInputElementDescs = nullptr;
//    inputLayoutDescDepth.NumElements = 0;
//    desc.InputLayout = inputLayoutDescDepth;
//
//    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//
//    // ブレンド設定は通常不要、色出力は1つだけ
//    D3D12_BLEND_DESC blendDesc = {};
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//    desc.BlendState = blendDesc;
//
//    // ラスタライザー設定
//    D3D12_RASTERIZER_DESC rasterizer = {};
//    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
//    rasterizer.CullMode = D3D12_CULL_MODE_BACK;
//    rasterizer.DepthClipEnable = TRUE;
//    desc.RasterizerState = rasterizer;
//
//    // デプスステンシルは通常無効にしてOK（深度をテクスチャとして書き出すため）
//    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
//    depthStencil.DepthEnable = FALSE;
//    depthStencil.StencilEnable = FALSE;
//    desc.DepthStencilState = depthStencil;
//    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
//
//    // Render Target Format
//    // 深度をレンダーターゲットとして出力する場合はfloat形式を使うことが多いです
//    desc.NumRenderTargets = 1;
//    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 例えばR32_FLOAT
//
//    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//    desc.SampleDesc.Count = 1;
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psoDepth_));
//    assert(SUCCEEDED(hr));
//    psoDepth_->SetName(L"PSO_DepthPass");
//}
//
//void PSOManager::CreateSkyboxPSO()
//{
//    // BlendStateの設定 (スカイボックスは不透明なのでブレンドOFF)
//    D3D12_BLEND_DESC blendDesc{};
//    blendDesc.RenderTarget[0].BlendEnable = FALSE; // ブレンドOFF
//    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//
//    // RasterizerStateの設定 (重要)
//    D3D12_RASTERIZER_DESC rasterizerDesc{};
//    // スカイボックスは「内側」から見るので、表面(時計回り)をカリングする
//    rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT; // CULL_MODE_FRONT に変更
//    // 三角形の中を塗りつぶす
//    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
//
//    // DepthStencilStateの設定 (最重要)
//    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
//    // Depthの機能を有効化する
//    depthStencilDesc.DepthEnable = true;
//    // 深度書き込みはしない(Zero)
//    // スカイボックスが深度バッファを埋めると、他の全てが描画されなくなるため
//    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
//    // 比較関数はLessEqual
//    // VSの.xywwトリックで深度が1.0になる。クリア値の1.0と同じなので LESS だと通らない
//    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
//
//    // PSOを生成する
//    // psoDescSkybox_ と rootSignatureSkybox_ を使用
//    psoDescSkybox_.pRootSignature = rootSignatureManager_->GetSkyboxRootSignature();
//    psoDescSkybox_.InputLayout = inputLayoutDescSkybox_; // スカイボックス用レイアウト
//    psoDescSkybox_.VS = { vsBlobSkybox_->GetBufferPointer(),
//    vsBlobSkybox_->GetBufferSize() }; // スカイボックス用VS
//    psoDescSkybox_.PS = { psBlobSkybox_->GetBufferPointer(),
//    psBlobSkybox_->GetBufferSize() }; // スカイボックス用PS
//    // DepthStencilの設定
//    psoDescSkybox_.DepthStencilState = depthStencilDesc;
//    psoDescSkybox_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
//    psoDescSkybox_.BlendState = blendDesc;
//    psoDescSkybox_.RasterizerState = rasterizerDesc;
//    // 書き込むRTVの情報
//    psoDescSkybox_.NumRenderTargets = 1;
//    psoDescSkybox_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//    // トポロジは三角形
//    psoDescSkybox_.PrimitiveTopologyType =
//        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//    // その他
//    psoDescSkybox_.SampleDesc.Count = 1;
//    psoDescSkybox_.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//
//    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDescSkybox_,
//        IID_PPV_ARGS(&psoSkybox_)); 
//    assert(SUCCEEDED(hr));
//}







void PSOManager::Initialize(
    ID3D12Device* device,
    ShaderManager* shaderManager,
    RootSignatureManager* rootSignatureManager)
{
    device_ = device;
    shaderManager_ = shaderManager;
    rootSignatureManager_ = rootSignatureManager;

    // --- 旧CreateInputLayout()の処理をここに移動 ---
    // "Default3D"用のレイアウトを構築
    inputElementsDefault_ = {
          { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
          { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
          { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    inputLayoutDescDefault_.pInputElementDescs = inputElementsDefault_.data();
    inputLayoutDescDefault_.NumElements = static_cast<UINT>(inputElementsDefault_.size());

    // --- "Skinning"用のレイアウト (スキニングメッシュ用) ---
    // (POSITION, TEXCOORD, NORMAL, WEIGHT, INDEX)
    // (※ヘッダーファイルへの
    //    inputLayoutDescSkinning_ と inputElementsSkinning_ の追加を忘れずに)
    inputElementsSkinning_ = {
        // Slot 0: 頂点情報
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        // Slot 1: スキニング情報
        { "WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "INDEX", 0, DXGI_FORMAT_R32G32B32A32_SINT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    inputLayoutDescSkinning_.pInputElementDescs = inputElementsSkinning_.data();
    inputLayoutDescSkinning_.NumElements = static_cast<UINT>(inputElementsSkinning_.size());

    // "Particle"用のレイアウトを構築
    inputElementsParticle_ = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } // ★これを追加
    };
    inputLayoutDescParticle_.pInputElementDescs = inputElementsParticle_.data();
    inputLayoutDescParticle_.NumElements = static_cast<UINT>(inputElementsParticle_.size());

    // --- "Depth"用のレイアウト ---
    // デプスプリパスは、通常メッシュ("Default3D")のレイアウトを使い回します。
    // (もしスキニングメッシュのデプスプリパスが必要な場合は、
    //  "SkinningDepth"のようなレイアウトを別途定義するか、GetInputLayout側で
    //  ロジックを組む必要がありますが、まずはこれで十分です)
    inputLayoutDescDepth_ = inputLayoutDescDefault_;

    // --- ★"Skybox"用のレイアウト (追加) ---
    inputElementsSkybox_ = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    inputLayoutDescSkybox_.pInputElementDescs = inputElementsSkybox_.data();
    inputLayoutDescSkybox_.NumElements = static_cast<UINT>(inputElementsSkybox_.size());

    // 旧InitializeにあったCreate...PSO()の呼び出しは全て削除する
}

ID3D12PipelineState* PSOManager::GetPSO(const std::string& psoName)
{
    // 1. キャッシュを検索
    if (auto it = psoCache_.find(psoName); it != psoCache_.end()) {
        return it->second.Get(); // 見つかった
    }

    // 2. 見つからないので生成 (オンデマンド)
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso = CreatePSO(psoName);
    assert(pso != nullptr);

    // 3. キャッシュに保存して返す
    psoCache_[psoName] = pso;
    return pso.Get();
}

// PSOのオンデマンド生成
Microsoft::WRL::ComPtr<ID3D12PipelineState> PSOManager::CreatePSO(const std::string& psoName)
{
    // === 1. JSONから設定を読み込む ===
    PSODescription desc = LoadPSODefinition(psoName);

    // === 2. 依存リソースを取得 (キャッシュが効く) ===
    std::wstring vsPath(desc.VertexShader.begin(), desc.VertexShader.end());
    std::wstring psPath(desc.PixelShader.begin(), desc.PixelShader.end());

    IDxcBlob* vsBlob = shaderManager_->GetShader(vsPath, L"vs_6_0");

    IDxcBlob* psBlob = nullptr;
    if (!psPath.empty()) {
        psBlob = shaderManager_->GetShader(psPath, L"ps_6_0");
    }

    ID3D12RootSignature* rootSig = rootSignatureManager_->GetRootSignature(desc.RootSignature);

    // === 3. 文字列をD3D12の構造体に変換 ===
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };

    if (psBlob) {
        psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    }
    else {
        psoDesc.PS = {}; // Pixel Shaderなし
    }

    psoDesc.InputLayout = GetInputLayout(desc.InputLayout);
    psoDesc.BlendState = GetBlendState(desc.BlendState);
    psoDesc.RasterizerState = GetRasterizerState(desc.RasterizerState);
    psoDesc.DepthStencilState = GetDepthStencilState(desc.DepthStencilState);
    psoDesc.PrimitiveTopologyType = GetTopologyType(desc.Topology);

    // ★★★ エラー修正箇所 ★★★

    // RTV/DSVフォーマットを先に取得
    DXGI_FORMAT rtvFormat = GetRTVFormat(desc.RTVFormat0);
    psoDesc.DSVFormat = GetDSVFormat(desc.DSVFormat);

    // RTVFormat が "UNKNOWN" かどうかで NumRenderTargets を設定
    if (rtvFormat == DXGI_FORMAT_UNKNOWN)
    {
        // RTVを使わない (Depth.json など)
        psoDesc.NumRenderTargets = 0;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    }
    else
    {
        // RTVを1つ使う (Standard3D.json など)
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = rtvFormat;
    }

    // ★★★ 修正ここまで ★★★

    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // === 4. PSOを生成 ===
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    assert(SUCCEEDED(hr));

    return pso;
}

PSODescription PSOManager::LoadPSODefinition(const std::string& psoName)
{
    std::string filePath = "Resources/json/PSODefinitions/" + psoName + ".json";

    std::ifstream file(filePath);
    if (!file.is_open()) {
        assert(false && "Failed to open PSO definition file.");
    }

    nlohmann::json json;
    file >> json;
    file.close();

    // PSODescription 構造体にマッピングする
    // ★修正: json["Key"] ではなく json.value("Key", default) を使う
    PSODescription desc; // デフォルトコンストラクタで初期化

    // 必須項目 (これらがないJSONはアサート)
    assert(json.contains("RootSignature") && "RootSignature is missing in JSON");
    desc.RootSignature = json["RootSignature"];

    assert(json.contains("VertexShader") && "VertexShader is missing in JSON");
    desc.VertexShader = json["VertexShader"];

    // 任意項目 (キーが存在しなければ PSODescription のデフォルト値が使われる)
    desc.PixelShader = json.value("PixelShader", desc.PixelShader);
    desc.InputLayout = json.value("InputLayout", desc.InputLayout);
    desc.BlendState = json.value("BlendState", desc.BlendState);
    desc.RasterizerState = json.value("RasterizerState", desc.RasterizerState);
    desc.DepthStencilState = json.value("DepthStencilState", desc.DepthStencilState);
    desc.Topology = json.value("Topology", desc.Topology);
    desc.RTVFormat0 = json.value("RTVFormat", desc.RTVFormat0); // JSON側は"RTVFormat"
    desc.DSVFormat = json.value("DSVFormat", desc.DSVFormat);

    // ★PixelShaderが空文字列の場合、明示的に "null" ではなく空にする
    // (nlohmann::json が "" を null と解釈することがあるため)
    if (json.contains("PixelShader") && json["PixelShader"].is_null()) {
        desc.PixelShader = "";
    }

    return desc;
}

// --- ヘルパー関数の実装例 ---
D3D12_BLEND_DESC PSOManager::GetBlendState(const std::string& name)
{
    // --- "AlphaBlend" ---
    // (旧Create3DPSOなど。一般的な半透明合成)
    if (name == "AlphaBlend") {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // --- "Additive" ---
    // (パーティクルなどで使う加算合成)
    if (name == "Additive") {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // 加算
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    if (name == "Subtract") {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // 減算
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        // Alpha
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // --- "Multiply" --- (kBlendModeMultily に対応)
    if (name == "Multiply") {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO; // 乗算
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR; // 乗算
        // Alpha
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // --- "Screen" --- (kBlendModeScreen に対応)
    if (name == "Screen") {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR; // スクリーン
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // スクリーン
        // Alpha
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // --- "Exclusion" --- (kBlendModeExclusion に対応)
    if (name == "Exclusion") {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR; // 除外
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_COLOR; // 除外
        // Alpha
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // --- "Opaque" ---
    // (デフォルト。不透明。ブレンドなし)
    if (name == "Opaque") {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = FALSE; // ブレンドしない
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // --- ★【追加】"BloomCombine" ---
    // (Bloomの合成用。Src=ONE, Dest=ONE)
    if (name == "BloomCombine") {
        D3D12_BLEND_DESC blendDesc{};
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

        return blendDesc;
    }

    // デフォルト (Opaqueと同じ)
    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return blendDesc;
}

D3D12_RASTERIZER_DESC PSOManager::GetRasterizerState(const std::string& name)
{
    // --- "BackCullSolid" ---
    // (旧Create3DPSOなど。裏面カリング、塗りつぶし)
    if (name == "BackCullSolid") {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        return rasterizerDesc;
    }

    // --- "Wireframe" ---
    // (旧Create3DWireframePSO。カリングなし、ワイヤーフレーム)
    if (name == "Wireframe") {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        return rasterizerDesc;
    }

    // --- "NoCullSolid" ---
    // (GridやSkybox用。カリングなし、塗りつぶし)
    if (name == "NoCullSolid") {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        return rasterizerDesc;
    }

    // --- "FrontCullSolid" ---
    // (Skyboxの裏返し対策など。前面カリング、塗りつぶし)
    if (name == "FrontCullSolid") {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        return rasterizerDesc;
    }

    if (name == "LineAA") {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK; // "BackCullSolid"と同じ
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // "BackCullSolid"と同じ
        rasterizerDesc.AntialiasedLineEnable = true; // ★ここがtrue
        return rasterizerDesc;
    }

    // デフォルト (BackCullSolidと同じ)
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    return rasterizerDesc;
}

D3D12_DEPTH_STENCIL_DESC PSOManager::GetDepthStencilState(const std::string& name)
{
    // "Default": 深度テストあり、深度書き込みあり (Create3DPSO, CreateSkinningPSO)
    if (name == "Default") {
        D3D12_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        return desc;
    }

    // "ReadOnly": 深度テストあり、深度書き込みなし (Grid, Particle, Skybox)
    if (name == "ReadOnly") {
        D3D12_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込まない
        desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // Skyboxのため LESS_EQUAL
        return desc;
    }

    // "DepthOnly": デプスプリパス用 (★CreateDepthPSO に対応)
    if (name == "DepthOnly") {
        D3D12_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 深度は書き込む
        desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        return desc;
    }

    // "Off": 深度テストなし (Fullscreen)
    if (name == "Off") {
        D3D12_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = false;
        return desc;
    }

    // デフォルト (Defaultと同じ)
    D3D12_DEPTH_STENCIL_DESC desc{};
    desc.DepthEnable = true;
    desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    return desc;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE PSOManager::GetTopologyType(const std::string& name)
{
    if (name == "Triangle") {
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }

    // (元の CreateLinePSO 用)
    if (name == "Line") {
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    }

    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

DXGI_FORMAT PSOManager::GetRTVFormat(const std::string& name)
{
    if (name == "R8G8B8A8_UNORM_SRGB") {
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    }
    // (HDRやBloom用)
    if (name == "R16G16B16A16_FLOAT") {
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    }
    // デプスプリパス用 (RTVなし)
    if (name == "UNKNOWN") {
        return DXGI_FORMAT_UNKNOWN;
    }
    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
}

DXGI_FORMAT PSOManager::GetDSVFormat(const std::string& name)
{
    if (name == "D24_UNORM_S8_UINT") {
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    }
    // ポストエフェクトなど (DSVなし)
    if (name == "UNKNOWN") {
        return DXGI_FORMAT_UNKNOWN;
    }
    return DXGI_FORMAT_D24_UNORM_S8_UINT;
}

D3D12_INPUT_LAYOUT_DESC PSOManager::GetInputLayout(const std::string& name)
{
    if (name == "Default3D") {
        return inputLayoutDescDefault_; // Initializeで構築済み
    }
    if (name == "Skinning") {
        return inputLayoutDescSkinning_; // Initializeで構築済み
    }
    if (name == "Particle") {
        return inputLayoutDescParticle_; // Initializeで構築済み
    }
    if (name == "Skybox") {
        return inputLayoutDescSkybox_; // Initializeで構築済み
    }
    if (name == "Depth") {
        // "Depth" は "Default3D" のレイアウトを流用
        return inputLayoutDescDepth_;
    }
    if (name == "Fullscreen") {
        // フルスクリーンクアッド(ポストエフェクト)は頂点入力なし
        return {};
    }

    // 不明な名前が指定された
    assert(false && "Unknown InputLayout name specified.");
    return inputLayoutDescDefault_; // とりあえずデフォルトを返す
}