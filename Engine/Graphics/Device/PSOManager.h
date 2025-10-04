#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <dxcapi.h>
#include <memory>
#include <vector>
#include <unordered_map>  

#include "BlendMode.h"

class RootSignatureManager;

class PSOManager
{
public:
    // 初期化
    void Initialize(
        ID3D12Device* device,
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler,
        RootSignatureManager* rootSignatureManager
    );

    // 全てのパーティクル用パイプラインステートオブジェクトを生成
    void CreateAllParticlePipelines();

    // 通常の3D描画用PSO
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso3D_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso3DWireframe_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoGrid_;
    // 線描画用PSO
    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoLine_;
    // 単一のパーティクル描画用PSO
    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoParticle_;
    // 各ブレンドモードに対応するパーティクル用PSOを格納するマップ
    std::unordered_map<BlendMode, Microsoft::WRL::ComPtr<ID3D12PipelineState>> psoParticles_;
    // PSOManagerにbloom用PSOを追加
    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoExtract_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoBlurX_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoBlurY_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoBloomCombine_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoFullscreen_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoDepth_;

private:
    // 入力レイアウトを生成
    void CreateInputLayout();
    // シェーダーをコンパイル
    void CompileShaders(IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler);
    // 通常のグラフィックスパイプラインステート記述子を設定
    void Create3DPSO();
    void Create3DWireframePSO();
    void CreateGridPSO();
    // 線描画用パイプラインステート記述子を設定
    void CreateLinePSO();
    // パーティクル描画用パイプラインステート記述子を設定
    void CreateParticlePSO(BlendMode blendMode);

    void CreateFullscreenPSO();
    void CreateBrightnessExtractPSO();
    void CreateBloomBlurVerticalPSO();
    void CreateBloomBlurHorizontalPSO();
    void CreateBloomCombinePSO();
    void CreatePostEffectPassPSO();
    void CreateDepthPSO();

    ID3D12Device* device_;
    RootSignatureManager* rootSignatureManager_;

    // シェーダーバイナリ
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob3D_;
    Microsoft::WRL::ComPtr<IDxcBlob> psBlob3D_;
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlobLine_;
    Microsoft::WRL::ComPtr<IDxcBlob> psBlobLine_;
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlobParticle_;
    Microsoft::WRL::ComPtr<IDxcBlob> psBlobParticle_;
    Microsoft::WRL::ComPtr<IDxcBlob> psBlobExtract_;
    Microsoft::WRL::ComPtr<IDxcBlob> psBlobBlurX_;
    Microsoft::WRL::ComPtr<IDxcBlob> psBlobBlurY_;
    Microsoft::WRL::ComPtr<IDxcBlob> psBlobBloomCombine_;
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlobFullscreen_;
    Microsoft::WRL::ComPtr<IDxcBlob> psBlobFullscreen_;
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlobDepth_;
    Microsoft::WRL::ComPtr<IDxcBlob> psBlobDepth_;

    // パイプラインステート記述子および入力レイアウト関連
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc_{}; 
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescParticle_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescDepth_{};
    D3D12_INPUT_ELEMENT_DESC inputElementDesc_[3] = {};
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescsParticle_;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc3d_{};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLine_{};    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescParticle_{};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc3dWireFrame_{};
};

