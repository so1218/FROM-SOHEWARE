#pragma once
#include "BlendMode.h"

namespace FE
{

class RootSignatureManager;
class ShaderManager;

// JSONで指定するPSO設定
struct PSODescription
{
    std::string RootSignature;
    std::string VertexShader;
    std::string PixelShader;
    std::string ComputeShader;
    std::string InputLayout;
    std::string BlendState;
    std::string RasterizerState;
    std::string DepthStencilState;
    std::string Topology = "Triangle";                  
    std::vector<std::string> RTVFormats;
    std::string DSVFormat = "D24_UNORM_S8_UINT";        
};

class PSOManager
{
public:
    void Initialize(
        ID3D12Device* device,
        ShaderManager* shaderManager,
        RootSignatureManager* rootSignatureManager
    );

    // PSO を名前で取得
    ID3D12PipelineState* GetPSO(const std::string& psoName);

private:
    // PSOを必要時に生成
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSO(const std::string& psoName);

    // データ変換
    PSODescription LoadPSODefinition(const std::string& psoName); 
    D3D12_INPUT_LAYOUT_DESC GetInputLayout(const std::string& name);
    D3D12_BLEND_DESC GetBlendState(const std::string& name);
    D3D12_RASTERIZER_DESC GetRasterizerState(const std::string& name);
    D3D12_DEPTH_STENCIL_DESC GetDepthStencilState(const std::string& name);
    D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType(const std::string& name);
    DXGI_FORMAT GetRTVFormat(const std::string& name);
    DXGI_FORMAT GetDSVFormat(const std::string& name);

    // メンバ変数
    ID3D12Device* device_ = nullptr;
    ShaderManager* shaderManager_ = nullptr;
    RootSignatureManager* rootSignatureManager_ = nullptr;

    // PSOのキャッシュ
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> psoCache_;

    // 入力レイアウト
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescDefault_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescSkinning_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescDepth_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescSkybox_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescTrail_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescLine_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescGrass_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescSkydome_{};
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescTerrain_{};

    // 入力要素（レイアウトを構成する配列の実データ）
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsDefault_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsSkinning_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsDepth_; 
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsSkybox_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsTrail_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsLine_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsGrass_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsSkydome_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsTerrain_;
};

}