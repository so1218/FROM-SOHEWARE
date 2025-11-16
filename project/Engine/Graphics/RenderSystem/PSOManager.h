#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <dxcapi.h>
#include <memory>
#include <vector>
#include <unordered_map>  
#include <string>

#include "BlendMode.h"
#include "ShaderManager.h"

class RootSignatureManager;

// JSONから読み込む設定
struct PSODescription {
    std::string RootSignature;
    std::string VertexShader;
    std::string PixelShader;
    std::string InputLayout;
    std::string BlendState;
    std::string RasterizerState;
    std::string DepthStencilState;
    std::string Topology = "Triangle"; // デフォルト値
    std::string RTVFormat0 = "R8G8B8A8_UNORM_SRGB"; // 追加
    std::string DSVFormat = "D24_UNORM_S8_UINT";
};

class PSOManager
{
public:
    void Initialize(
        ID3D12Device* device,
        ShaderManager* shaderManager, // インスタンスを受け取る
        RootSignatureManager* rootSignatureManager
    );

    // PSOを名前で取得する (これが唯一の公開I/F)
    // 例: GetPSO("Standard3D"), GetPSO("Wireframe")
    ID3D12PipelineState* GetPSO(const std::string& psoName);

private:
    // PSOをオンデマンドで生成
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(const std::string& psoName);

    // --- データ変換ヘルパー ---
    // JSONの文字列をD3D12の構造体に変換する
    PSODescription LoadPSODefinition(const std::string& psoName); // JSONをパースする
    D3D12_INPUT_LAYOUT_DESC GetInputLayout(const std::string& name);
    D3D12_BLEND_DESC GetBlendState(const std::string& name);
    D3D12_RASTERIZER_DESC GetRasterizerState(const std::string& name);
    D3D12_DEPTH_STENCIL_DESC GetDepthStencilState(const std::string& name);
    D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType(const std::string& name);
    DXGI_FORMAT GetRTVFormat(const std::string& name); // ヘルパーを新設
    DXGI_FORMAT GetDSVFormat(const std::string& name);
    // ... 他の変換関数 (RTVFormatなど)

    // --- メンバ変数 ---
    ID3D12Device* device_ = nullptr;
    ShaderManager* shaderManager_ = nullptr;
    RootSignatureManager* rootSignatureManager_ = nullptr;

    // PSOのキャッシュ
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> psoCache_;

    // 1. 各レイアウトのデスクリプタ
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescDefault_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescSkinning_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescParticle_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescDepth_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescSkybox_{};
    // ...
   // 2. 各レイアウトの要素配列 (vectorで実体を保持)
    // ※元の inputElementDesc_[] は Default3D 用と仮定
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsDefault_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsSkinning_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsParticle_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsDepth_; // (Depth用に新設)
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsSkybox_;
};

//
//class PSOManager
//{
//public:
//    // 初期化
//    void Initialize(
//        ID3D12Device* device,
//        IDxcUtils* dxcUtils,
//        IDxcCompiler3* dxcCompiler,
//        IDxcIncludeHandler* includeHandler,
//        RootSignatureManager* rootSignatureManager
//    );
//
//    // 全てのパーティクル用パイプラインステートオブジェクトを生成
//    void CreateAllParticlePipelines();
//
//    // 通常の3D描画用PSO
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso3D_;
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso3DWireframe_;
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoSkinning_;
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoGrid_;
//    // 線描画用PSO
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoLine_;
//    // 単一のパーティクル描画用PSO
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoParticle_;
//    // 各ブレンドモードに対応するパーティクル用PSOを格納するマップ
//    std::unordered_map<BlendMode, Microsoft::WRL::ComPtr<ID3D12PipelineState>> psoParticles_;
//    // PSOManagerにbloom用PSOを追加
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoExtract_;
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoBlurX_;
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoBlurY_;
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoBloomCombine_;
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoFullscreen_;
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoDepth_;
//    Microsoft::WRL::ComPtr<ID3D12PipelineState> psoSkybox_;
//
//private:
//    // 入力レイアウトを生成
//    void CreateInputLayout();
//    // シェーダーをコンパイル
//    void CompileShaders(IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler);
//    // 通常のグラフィックスパイプラインステート記述子を設定
//    void Create3DPSO();
//    void Create3DWireframePSO();
//    void CreateSkinningPSO();
//    void CreateGridPSO();
//    // 線描画用パイプラインステート記述子を設定
//    void CreateLinePSO();
//    // パーティクル描画用パイプラインステート記述子を設定
//    void CreateParticlePSO(BlendMode blendMode);
//
//    void CreateFullscreenPSO();
//    void CreateBrightnessExtractPSO();
//    void CreateBloomBlurVerticalPSO();
//    void CreateBloomBlurHorizontalPSO();
//    void CreateBloomCombinePSO();
//    void CreatePostEffectPassPSO();
//    void CreateDepthPSO();
//    void CreateSkyboxPSO();
//
//    ID3D12Device* device_;
//    RootSignatureManager* rootSignatureManager_;
//
//    // シェーダーバイナリ
//    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob3D_;
//    Microsoft::WRL::ComPtr<IDxcBlob> psBlob3D_;
//    Microsoft::WRL::ComPtr<IDxcBlob> vsBlobSkinning_;
//    Microsoft::WRL::ComPtr<IDxcBlob> vsBlobLine_;
//    Microsoft::WRL::ComPtr<IDxcBlob> psBlobLine_;
//    Microsoft::WRL::ComPtr<IDxcBlob> vsBlobParticle_;
//    Microsoft::WRL::ComPtr<IDxcBlob> psBlobParticle_;
//    Microsoft::WRL::ComPtr<IDxcBlob> psBlobExtract_;
//    Microsoft::WRL::ComPtr<IDxcBlob> psBlobBlurX_;
//    Microsoft::WRL::ComPtr<IDxcBlob> psBlobBlurY_;
//    Microsoft::WRL::ComPtr<IDxcBlob> psBlobBloomCombine_;
//    Microsoft::WRL::ComPtr<IDxcBlob> vsBlobFullscreen_;
//    Microsoft::WRL::ComPtr<IDxcBlob> psBlobFullscreen_;
//    Microsoft::WRL::ComPtr<IDxcBlob> vsBlobDepth_;
//    Microsoft::WRL::ComPtr<IDxcBlob> psBlobDepth_;
//    Microsoft::WRL::ComPtr<IDxcBlob> vsBlobSkybox_; 
//    Microsoft::WRL::ComPtr<IDxcBlob> psBlobSkybox_;
//
//    // パイプラインステート記述子および入力レイアウト関連
//    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc_{}; 
//    D3D12_INPUT_LAYOUT_DESC inputLayoutDescParticle_{};
//    D3D12_INPUT_LAYOUT_DESC inputLayoutDescDepth_{};
//    D3D12_INPUT_LAYOUT_DESC inputLayoutDescSkybox_{};
//    D3D12_INPUT_ELEMENT_DESC inputElementDesc_[5] = {};
//    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescsParticle_;
//    D3D12_INPUT_ELEMENT_DESC inputElementDescSkybox_[1] = {};
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc3d_{};
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescSkinning_{};
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescLine_{};    
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescParticle_{};
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc3dWireFrame_{};
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescSkybox_{};
//};
//
