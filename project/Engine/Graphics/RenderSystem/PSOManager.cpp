#include "PSOManager.h"
#include "ShaderManager.h"
#include "RootSignatureManager.h"

#include <fstream>
#include <cassert>
#include <json.hpp>

void PSOManager::Initialize(
    ID3D12Device* device,
    ShaderManager* shaderManager,
    RootSignatureManager* rootSignatureManager)
{
    device_ = device;
    shaderManager_ = shaderManager;
    rootSignatureManager_ = rootSignatureManager;

    // Default3D
    inputElementsDefault_ =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    inputLayoutDescDefault_ = { inputElementsDefault_.data(), (UINT)inputElementsDefault_.size() };

    // Skinning 
    inputElementsSkinning_ =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "WEIGHT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "INDEX",    0, DXGI_FORMAT_R32G32B32A32_SINT,  1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    inputLayoutDescSkinning_ = { inputElementsSkinning_.data(), (UINT)inputElementsSkinning_.size() };

    // Depth 
    inputLayoutDescDepth_ = inputLayoutDescDefault_;

    // Skybox
    inputElementsSkybox_ =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    inputLayoutDescSkybox_ = { inputElementsSkybox_.data(), (UINT)inputElementsSkybox_.size() };

    // Trail
    inputElementsTrail_ =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    inputLayoutDescTrail_ = { inputElementsTrail_.data(), (UINT)inputElementsTrail_.size() };

    // Line
    inputElementsLine_ =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    inputLayoutDescLine_ = { inputElementsLine_.data(), (UINT)inputElementsLine_.size() };

    // Grass
    inputElementsGrass_ =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    inputLayoutDescGrass_ = { inputElementsGrass_.data(), (UINT)inputElementsGrass_.size() };
}

ID3D12PipelineState* PSOManager::GetPSO(const std::string& psoName)
{
    // キャッシュにあれば返す
    if (auto it = psoCache_.find(psoName); it != psoCache_.end())
    {
        return it->second.Get();
    }

    // なければ生成してキャッシュ
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso = CreatePSO(psoName);
    assert(pso != nullptr);

    psoCache_[psoName] = pso;
    return pso.Get();
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PSOManager::CreatePSO(const std::string& psoName)
{
    // JSON 読み込み
    PSODescription desc = LoadPSODefinition(psoName);

    // シェーダ・ルートシグネチャ取得
    std::wstring vsPath(desc.VertexShader.begin(), desc.VertexShader.end());
    std::wstring psPath(desc.PixelShader.begin(), desc.PixelShader.end());
    IDxcBlob* vsBlob = shaderManager_->GetShader(vsPath, L"vs_6_0");
    IDxcBlob* psBlob = psPath.empty() ? nullptr : shaderManager_->GetShader(psPath, L"ps_6_0");
    ID3D12RootSignature* rootSig = rootSignatureManager_->GetRootSignature(desc.RootSignature);

    // JSON の文字列 → D3D12 設定へ反映
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = psBlob ? D3D12_SHADER_BYTECODE{ psBlob->GetBufferPointer(), psBlob->GetBufferSize() } : D3D12_SHADER_BYTECODE{};

    psoDesc.InputLayout = GetInputLayout(desc.InputLayout);
    psoDesc.BlendState = GetBlendState(desc.BlendState);
    psoDesc.RasterizerState = GetRasterizerState(desc.RasterizerState);
    psoDesc.DepthStencilState = GetDepthStencilState(desc.DepthStencilState);
    psoDesc.PrimitiveTopologyType = GetTopologyType(desc.Topology);

    // RTV / DSV 設定
    psoDesc.DSVFormat = GetDSVFormat(desc.DSVFormat);

    if (desc.RTVFormats.empty() ||
        (desc.RTVFormats.size() == 1 && GetRTVFormat(desc.RTVFormats[0]) == DXGI_FORMAT_UNKNOWN))
    {
        // RTVなし（Depthのみのパスなど）
        psoDesc.NumRenderTargets = 0;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    }
    else
    {
        // RTV複数枚（G-Bufferなど）または1枚
        psoDesc.NumRenderTargets = static_cast<UINT>(desc.RTVFormats.size());
        for (UINT i = 0; i < psoDesc.NumRenderTargets; ++i)
        {
            psoDesc.RTVFormats[i] = GetRTVFormat(desc.RTVFormats[i]);
        }
    }

    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // PSO 生成
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    assert(SUCCEEDED(hr));
    return pso;
}

PSODescription PSOManager::LoadPSODefinition(const std::string& psoName)
{
    std::string filePath = "Assets/Data/PSODefinitions/" + psoName + ".json";

    // JSON読み込み
    std::ifstream file(filePath);
    if (!file.is_open()) assert(false && "Failed to open PSO definition file.");

    nlohmann::json json;
    file >> json;

    PSODescription desc;   // 既定値入り

    // 必須キー
    assert(json.contains("RootSignature") && "Missing RootSignature");
    assert(json.contains("VertexShader") && "Missing VertexShader");
    desc.RootSignature = json["RootSignature"];
    desc.VertexShader = json["VertexShader"];

    if (json.contains("PixelShader") && !json["PixelShader"].is_null())
    {
        desc.PixelShader = json["PixelShader"].get<std::string>();
    }
    else
    {
        desc.PixelShader = ""; // null または未定義なら空文字
    }

    // 任意キー（無ければ既定値）
    desc.InputLayout = json.value("InputLayout", desc.InputLayout);
    desc.BlendState = json.value("BlendState", desc.BlendState);
    desc.RasterizerState = json.value("RasterizerState", desc.RasterizerState);
    desc.DepthStencilState = json.value("DepthStencilState", desc.DepthStencilState);
    desc.Topology = json.value("Topology", desc.Topology);
    if (json.contains("RTVFormats") && json["RTVFormats"].is_array())
    {
        // "RTVFormats" : ["Format1", "Format2"] のように配列で指定された場合
        for (const auto& fmt : json["RTVFormats"])
        {
            desc.RTVFormats.push_back(fmt.get<std::string>());
        }
    }
    desc.DSVFormat = json.value("DSVFormat", desc.DSVFormat);

    return desc;
}

D3D12_BLEND_DESC PSOManager::GetBlendState(const std::string& name)
{
    // 透明合成
    if (name == "AlphaBlend")
    {
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

    // 加算
    if (name == "Additive" || name == "Add")
    {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // 減算
    if (name == "Subtract")
    {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // 乗算
    if (name == "Multiply")
    {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // スクリーン
    if (name == "Screen")
    {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // 除外
    if (name == "Exclusion")
    {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_COLOR;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }

    // Bloom 合成
    if (name == "BloomCombine")
    {
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

    // UI用
    if (name == "AlphaBlendUI")
    {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;

        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

        // 書き込みマスクをRGBのみに制限する（Alphaを除外）
        blendDesc.RenderTarget[0].RenderTargetWriteMask =
            D3D12_COLOR_WRITE_ENABLE_RED |
            D3D12_COLOR_WRITE_ENABLE_GREEN |
            D3D12_COLOR_WRITE_ENABLE_BLUE;

        return blendDesc;
    }

    // 不透明（ブレンドなし）
    if (name == "Opaque")
    {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = FALSE; // ブレンドしない
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        return blendDesc;
    }


    // 不透明
    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return blendDesc;
}

D3D12_RASTERIZER_DESC PSOManager::GetRasterizerState(const std::string& name)
{
    // 裏面カリング
    if (name == "BackCullSolid")
    {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        return rasterizerDesc;
    }

    // ワイヤーフレーム
    if (name == "Wireframe")
    {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        return rasterizerDesc;
    }

    // カリングなし
    if (name == "NoCullSolid")
    {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        return rasterizerDesc;
    }

    // 前面カリング
    if (name == "FrontCullSolid")
    {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        return rasterizerDesc;
    }

    // アウトライン用 (前面カリング + 深度バイアス)
    if (name == "FrontCullBias")
    {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

        // Zファイティング対策
        rasterizerDesc.DepthBias = 100;
        rasterizerDesc.SlopeScaledDepthBias = 1.0f;
        rasterizerDesc.DepthBiasClamp = 0.0f;

        return rasterizerDesc;
    }

    // 線のAA
    if (name == "LineAA")
    {
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.AntialiasedLineEnable = true;
        return rasterizerDesc;
    }

    // デフォルト（裏面カリング）
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    return rasterizerDesc;
}

D3D12_DEPTH_STENCIL_DESC PSOManager::GetDepthStencilState(const std::string& name)
{
    // 深度テスト+書き込み
    if (name == "Default")
    {
        D3D12_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        return desc;
    }

    // 深度読み取りのみ
    if (name == "ReadOnly")
    {
        D3D12_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        return desc;
    }

    // デプス前処理用
    if (name == "DepthOnly")
    {
        D3D12_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 深度は書き込む
        desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        return desc;
    }

    // 深度テストなし
    if (name == "Off")
    {
        D3D12_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = false;
        return desc;
    }

    // デフォルト
    D3D12_DEPTH_STENCIL_DESC desc{};
    desc.DepthEnable = true;
    desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    return desc;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE PSOManager::GetTopologyType(const std::string& name)
{
    if (name == "Triangle")
    {
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }
    if (name == "Line")
    {
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    }

    // 不明なら三角形
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

DXGI_FORMAT PSOManager::GetRTVFormat(const std::string& name)
{
    if (name == "R8G8B8A8_UNORM_SRGB")
    {
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    }
    if (name == "R16G16B16A16_FLOAT")
    {
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    }
    if (name == "R8G8B8A8_UNORM")
    { 
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    if (name == "R8_UNORM")
    {
        return DXGI_FORMAT_R8_UNORM;
    }
    if (name == "UNKNOWN")
    {
        return DXGI_FORMAT_UNKNOWN;
    }

    // 不明なら標準フォーマット
    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
}

DXGI_FORMAT PSOManager::GetDSVFormat(const std::string& name)
{
    if (name == "D24_UNORM_S8_UINT")
    {
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    }
    if (name == "D32_FLOAT")
    {
        return DXGI_FORMAT_D32_FLOAT;
    }
    if (name == "UNKNOWN")
    {
        return DXGI_FORMAT_UNKNOWN;
    }

    // 不明なら標準フォーマット
    return DXGI_FORMAT_D24_UNORM_S8_UINT;
}

D3D12_INPUT_LAYOUT_DESC PSOManager::GetInputLayout(const std::string& name)
{
    if (name == "None" || name == "")
    {
        return { nullptr, 0 };
    }
    if (name == "Default3D")
    {
        return inputLayoutDescDefault_;
    }
    if (name == "Skinning")
    {
        return inputLayoutDescSkinning_;
    }
    if (name == "Skybox")
    {
        return inputLayoutDescSkybox_;
    }
    if (name == "Depth")
    {
        return inputLayoutDescDepth_;
    }
    if (name == "Fullscreen")
    {
        return {};
    }
    if (name == "Trail")
    {
        return inputLayoutDescTrail_;
    }
    if (name == "Line")
    {
        return inputLayoutDescLine_;
    }
    if (name == "Grass")
    {
        return inputLayoutDescGrass_;
    }

    // 未定義のレイアウト
    assert(false && "Unknown InputLayout name.");
    return inputLayoutDescDefault_;
}