#include "RootSignatureBuilder.h"
#include "Logger.h"

void RootSignatureBuilder::AddCBV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;

    parameters_.push_back(param);
}

void RootSignatureBuilder::AddSRV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;

    parameters_.push_back(param);
}

void RootSignatureBuilder::AddUAV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;

    parameters_.push_back(param);
}

void RootSignatureBuilder::AddConstants(UINT shaderRegister, UINT num32BitValues, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.ShaderVisibility = visibility;
    param.Constants.ShaderRegister = shaderRegister;
    param.Constants.RegisterSpace = registerSpace;
    param.Constants.Num32BitValues = num32BitValues;

    parameters_.push_back(param);
}

void RootSignatureBuilder::AddDescriptorTableRange(
    D3D12_DESCRIPTOR_RANGE_TYPE type,
    UINT baseShaderRegister,
    UINT numDescriptors,
    D3D12_SHADER_VISIBILITY visibility,
    UINT registerSpace)
{
    // 1. レンジを定義
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = type;
    range.NumDescriptors = numDescriptors;
    range.BaseShaderRegister = baseShaderRegister;
    range.RegisterSpace = registerSpace;
    // D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND を使う
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // 2. 1つのレンジを持つテーブルとして、AddDescriptorTable を呼び出す
    //    (vector の初期化子リストを使用)
    AddDescriptorTable({ range }, visibility);
}

void RootSignatureBuilder::AddDescriptorTable(const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges, D3D12_SHADER_VISIBILITY visibility)
{
    // 1. レンジのコピーを内部ストレージに保存
    //    (Build時にこのデータへのポインタを使うため)
    descriptorRangeStorage_.push_back(ranges);

    // 2. ルートパラメータを設定
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.ShaderVisibility = visibility;
    // 最後にストレージに追加された vector の実体を指す
    param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descriptorRangeStorage_.back().size());
    param.DescriptorTable.pDescriptorRanges = descriptorRangeStorage_.back().data();

    parameters_.push_back(param);
}

void RootSignatureBuilder::AddStaticSampler(
    UINT shaderRegister,
    D3D12_FILTER filter,
    D3D12_TEXTURE_ADDRESS_MODE addressModeAll,
    D3D12_SHADER_VISIBILITY visibility,
    float maxLod)
{
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = filter;
    sampler.AddressU = addressModeAll;
    sampler.AddressV = addressModeAll;
    sampler.AddressW = addressModeAll;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 既存コードに合わせる
    sampler.MaxLOD = maxLod;
    sampler.ShaderRegister = shaderRegister;
    sampler.ShaderVisibility = visibility;

    staticSamplers_.push_back(sampler);
}

ComPtr<ID3D12RootSignature> RootSignatureBuilder::Build(
    ID3D12Device* device,
    D3D12_ROOT_SIGNATURE_FLAGS flags,
    const std::string& nameForLogging)
{
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.Flags = flags;
    desc.NumParameters = static_cast<UINT>(parameters_.size());
    desc.pParameters = parameters_.data(); // m_parameters が実体
    desc.NumStaticSamplers = static_cast<UINT>(staticSamplers_.size());
    desc.pStaticSamplers = staticSamplers_.data(); // m_staticSamplers が実体

    // シリアライズ
    ComPtr<ID3DBlob> signatureBlob = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&desc,
        D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

    if (FAILED(hr))
    {
        std::string errorMsg = "Failed to serialize " + nameForLogging + " root signature.";
        if (errorBlob)
        {
            errorMsg += "\n" + std::string(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        LOG_ERROR("{}", errorMsg);
        return nullptr;
    }

    // 生成
    ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    hr = device->CreateRootSignature(0,
        signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature));

    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create {} root signature. HRESULT: {:#x}", nameForLogging, hr);
        return nullptr;
    }

    // 成功
    return rootSignature;
}