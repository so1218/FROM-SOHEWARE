#include "RootSignatureManager.h"
#include "RootSignatureBuilder.h"
#include "Logger.h"

void RootSignatureManager::Initialize(ID3D12Device* device)
{
    device_ = device;
    LOG_INFO("RootSignatureManager Initialized.");
}

ID3D12RootSignature* RootSignatureManager::GetRootSignature(const std::string& name)
{
    // キャッシュチェック
    if (auto it = rootSignatureCache_.find(name); it != rootSignatureCache_.end())
    {
        return it->second.Get();
    }

    // なければ生成
    LOG_INFO("Creating RootSignature: {}", name);
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig = CreateRootSignature(name);
    assert(rootSig != nullptr);

    rootSignatureCache_[name] = rootSig;
    return rootSig.Get();
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureManager::CreateRootSignature(const std::string& name)
{
    RootSignatureBuilder builder;

    if (name == "3D")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(3, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_COMPARISON_FUNC_LESS_EQUAL);
        builder.AddStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "3D");
    }
    if (name == "Skinning")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(3, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_COMPARISON_FUNC_LESS_EQUAL);
        builder.AddStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Skinning");
    }
    if (name == "Sprite") 
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL); // t0: Texture

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Sprite");
    }
    if (name == "Outline")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_VERTEX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Outline");
    }

    if (name == "SkinningOutline")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_VERTEX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "SkinningOutline");
    }
    if (name == "Line")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX);
        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Line");
    }
    if (name == "Particle")
    {
        builder.AddSRV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Particle");
    }
    if (name == "PostProcess")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 8, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "PostProcess");
    }
    if (name == "Fullscreen")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Fullscreen");
    }
    if (name == "DepthExtract")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "DepthExtract");
    }
    if (name == "Skybox")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Skybox");
    }
    if (name == "Trail")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Trail");
    }
    if (name == "ShadowMap")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "ShadowMap");
    }
    if (name == "ShadowMapSkinning")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "ShadowMapSkinning");
    }

    // どれにも該当しない
    LOG_ERROR("Unknown RootSignature: {}", name);
    assert(false && "Unknown RootSignature name.");
    return nullptr;
}