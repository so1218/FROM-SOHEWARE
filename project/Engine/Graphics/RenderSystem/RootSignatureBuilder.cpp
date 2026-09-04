#include "pch.h"
#include "RootSignatureBuilder.h"
#include "Logger.h"

namespace FE
{

void RootSignatureBuilder::AddCBV(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility, uint32_t registerSpace)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;

    parameters_.push_back(param);
}

void RootSignatureBuilder::AddSRV(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility, uint32_t registerSpace)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;

    parameters_.push_back(param);
}

void RootSignatureBuilder::AddUAV(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility, uint32_t registerSpace)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;

    parameters_.push_back(param);
}

void RootSignatureBuilder::AddConstants(uint32_t shaderRegister, uint32_t num32BitValues, D3D12_SHADER_VISIBILITY visibility, uint32_t registerSpace)
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
    uint32_t baseShaderRegister,
    uint32_t numDescriptors,
    D3D12_SHADER_VISIBILITY visibility,
    uint32_t registerSpace)
{
    // 単一のデスクリプタレンジを作成してテーブルに追加
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = type;
    range.NumDescriptors = numDescriptors;
    range.BaseShaderRegister = baseShaderRegister;
    range.RegisterSpace = registerSpace;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    AddDescriptorTable({ range }, visibility);
}

void RootSignatureBuilder::AddDescriptorTable(const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges, D3D12_SHADER_VISIBILITY visibility)
{
    // レンジを内部ストレージに保持（Build時に参照するため）
    descriptorRangeStorage_.push_back(ranges);

    // ルートパラメータとして登録
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.ShaderVisibility = visibility;
    param.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(descriptorRangeStorage_.back().size());
    param.DescriptorTable.pDescriptorRanges = descriptorRangeStorage_.back().data();

    parameters_.push_back(param);
}

void RootSignatureBuilder::AddStaticSampler(
    uint32_t shaderRegister,
    D3D12_FILTER filter,
    D3D12_TEXTURE_ADDRESS_MODE addressModeAll,
    D3D12_SHADER_VISIBILITY visibility,
    D3D12_COMPARISON_FUNC comparisonFunc,
    float maxLod)
{
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = filter;
    sampler.AddressU = addressModeAll;
    sampler.AddressV = addressModeAll;
    sampler.AddressW = addressModeAll;
    sampler.ComparisonFunc = comparisonFunc;
    sampler.MaxLOD = maxLod;
    sampler.ShaderRegister = shaderRegister;
    sampler.ShaderVisibility = visibility;
    sampler.MaxAnisotropy = 1;

    // シャドウマップの場合、枠外は白（影なし）にしないと、マップ外が全部影になる可能性がある
    if (filter == D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR ||
        filter == D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT)
    {
        // 影用設定：枠外は白（1.0 = 奥）
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    }
    else
    {
        // 通常テクスチャ：枠外は透明黒（0.0）
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    }

    staticSamplers_.push_back(sampler);
}

ComPtr<ID3D12RootSignature> RootSignatureBuilder::Build(
    ID3D12Device* device,
    D3D12_ROOT_SIGNATURE_FLAGS flags,
    const std::string& nameForLogging)
{
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.Flags = flags;
    desc.NumParameters = static_cast<uint32_t>(parameters_.size());
    desc.pParameters = parameters_.data(); 
    desc.NumStaticSamplers = static_cast<uint32_t>(staticSamplers_.size());
    desc.pStaticSamplers = staticSamplers_.data();

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

}