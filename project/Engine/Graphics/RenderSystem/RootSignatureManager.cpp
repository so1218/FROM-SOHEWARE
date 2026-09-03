#include "pch.h"
#include "RootSignatureManager.h"
#include "RootSignatureBuilder.h"
#include "Logger.h"

namespace FE
{

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

    if (name == "Skinning" || name == "Instancing3D")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);    
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);    
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddCBV(3, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_ALL);   

        if (name == "Skinning")
        {
            builder.AddCBV(6, D3D12_SHADER_VISIBILITY_VERTEX);
        }
        else if (name == "Instancing3D")
        {
            builder.AddConstants(7, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        }
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 1, D3D12_SHADER_VISIBILITY_PIXEL);

        if (name == "Skinning")
        {
            builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 1, D3D12_SHADER_VISIBILITY_VERTEX); 
            builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        }

        if (name == "Instancing3D")
        {
            builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 1, D3D12_SHADER_VISIBILITY_VERTEX); 
        }

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_COMPARISON_FUNC_LESS_EQUAL);
        builder.AddStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, name);
    }
    if (name == "Sprite")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
            D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Sprite");
    }
    if (name == "OutlineSkinning")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(6, D3D12_SHADER_VISIBILITY_VERTEX);

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
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, UINT_MAX, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Particle");
    }
    if (name == "PostProcess")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 7, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "PostProcess");
    }
    if (name == "Fullscreen")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Fullscreen");
    }
    if (name == "BokehBlur")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "BokehBlur");
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
    if (name == "Skydome")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(6, D3D12_SHADER_VISIBILITY_PIXEL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Skydome");
    }
    if (name == "Terrain")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);    
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddCBV(3, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_ALL);    
        builder.AddCBV(10, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddSRV(10, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9, 1, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddStaticSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_COMPARISON_FUNC_LESS_EQUAL);
        builder.AddStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, name);
    }

    if (name == "Lightning")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Lightning");
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
    if (name == "ShadowMapInstanced")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);         
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX);         
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_VERTEX);         
        builder.AddConstants(7, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_VERTEX);         
        builder.AddConstants(9, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 1, D3D12_SHADER_VISIBILITY_VERTEX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "ShadowMapInstanced");
    }
    if (name == "ShadowMapDissolveInstanced")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);          
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX);          
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_ALL);             
        builder.AddConstants(7, 1, D3D12_SHADER_VISIBILITY_VERTEX); 
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_VERTEX);          
        builder.AddConstants(9, 1, D3D12_SHADER_VISIBILITY_VERTEX); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 1, D3D12_SHADER_VISIBILITY_VERTEX); 

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "ShadowMapDissolveInstanced");
    }
    if (name == "ShadowMapSkinning")
    {
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(6, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddConstants(9, 1, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "ShadowMapSkinning");
    }
    if (name == "ShadowMapSkinningDissolve")
    {
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(6, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddConstants(9, 1, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "ShadowMapSkinningDissolve");
    }
    if (name == "ShadowMapTerrain")
    {
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddConstants(9, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddSRV(9, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(10, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 1, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_VERTEX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, name);
    }
    if (name == "ShadowMapTreeFoliage")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddConstants(3, 2, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddConstants(5, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(6, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 1, D3D12_SHADER_VISIBILITY_PIXEL);

        builder.AddStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddStaticSampler(3, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "ShadowMapTreeFoliage");
    }
    if (name == "ShadowMapTreeTrunk")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddConstants(3, 2, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddConstants(5, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(6, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 1, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_VERTEX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "ShadowMapTreeTrunk");
    }
    if (name == "SSAO")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "SSAO");
    }
    if (name == "BilateralBlur")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "BilateralBlur");
    }
    if (name == "SSR_CS")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "SSR_CS");
    }
    if (name == "Grass")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);    
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_ALL);    
        builder.AddCBV(6, D3D12_SHADER_VISIBILITY_VERTEX); 
        builder.AddCBV(7, D3D12_SHADER_VISIBILITY_VERTEX); 
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_VERTEX); 
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_PIXEL);  

        builder.AddSRV(10, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 11, 1, D3D12_SHADER_VISIBILITY_VERTEX); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);   
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 12, 1, D3D12_SHADER_VISIBILITY_VERTEX); 

        builder.AddStaticSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_COMPARISON_FUNC_LESS_EQUAL);
        builder.AddStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_VERTEX); 
        builder.AddStaticSampler(3, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_VERTEX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_NONE, name);
    }
    if (name == "GrassCullingCS")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "GrassCullingCS");
    }
    if (name == "GrassGenerationCS")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "GrassGenerationCS");
    }
    if (name == "TreeFoliage")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);   
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_ALL);   
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_ALL);   
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_PIXEL); 

        builder.AddConstants(9, 2, D3D12_SHADER_VISIBILITY_VERTEX); 

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 11, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 12, 1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 13, 1, D3D12_SHADER_VISIBILITY_PIXEL); 

        builder.AddStaticSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_COMPARISON_FUNC_LESS_EQUAL);

        builder.AddStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_VERTEX); 

        builder.AddStaticSampler(3, D3D12_FILTER_ANISOTROPIC,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Foliage_Leaf");
    }
    if (name == "TreeTrunk")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);   
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddCBV(3, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_ALL);   
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddCBV(6, D3D12_SHADER_VISIBILITY_PIXEL); 
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_PIXEL); 

        builder.AddConstants(9, 2, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 11, 1, D3D12_SHADER_VISIBILITY_VERTEX);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);

        builder.AddStaticSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_COMPARISON_FUNC_LESS_EQUAL); 

        builder.AddStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_VERTEX); 

        builder.AddStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, name);
    }
    if (name == "TreeCullingCS")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddSRV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddUAV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddUAV(1, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "TreeCullingCS");
    }
    if (name == "Pebble")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL, 0);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL, 0);
        builder.AddCBV(3, D3D12_SHADER_VISIBILITY_PIXEL, 0);
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_PIXEL, 0);
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_PIXEL, 0);
        builder.AddSRV(10, D3D12_SHADER_VISIBILITY_VERTEX, 0);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, D3D12_SHADER_VISIBILITY_PIXEL);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_COMPARISON_FUNC_ALWAYS, D3D12_FLOAT32_MAX);

        builder.AddStaticSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_FLOAT32_MAX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Pebble");
    }

    if (name == "PebbleGenerationCS")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL, 0);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL, 0);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_ALL,
            D3D12_COMPARISON_FUNC_ALWAYS, D3D12_FLOAT32_MAX);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "PebbleGenerationCS");
    }

    if (name == "PebbleCullingCS")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL, 0);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL, 0);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, D3D12_SHADER_VISIBILITY_ALL, 0);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "PebbleCullingCS");
    }
    if (name == "Foliage")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL, 0);    
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL, 0);  
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_ALL, 0);    
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_ALL, 0);    
        builder.AddCBV(6, D3D12_SHADER_VISIBILITY_VERTEX, 0); 
        builder.AddCBV(8, D3D12_SHADER_VISIBILITY_PIXEL, 0);  

        builder.AddSRV(10, D3D12_SHADER_VISIBILITY_VERTEX, 0);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);  
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, D3D12_SHADER_VISIBILITY_VERTEX); 

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_VERTEX,
            D3D12_COMPARISON_FUNC_NEVER, D3D12_FLOAT32_MAX);

        builder.AddStaticSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_FLOAT32_MAX);

        builder.AddStaticSampler(3, D3D12_FILTER_ANISOTROPIC,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL,
            D3D12_COMPARISON_FUNC_ALWAYS, D3D12_FLOAT32_MAX);

        return builder.Build(device_, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Foliage");
    }
    if (name == "FoliageGenerationCS")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL, 0);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL, 0);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_ALL,
            D3D12_COMPARISON_FUNC_ALWAYS, D3D12_FLOAT32_MAX);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "FoliageGenerationCS");
    }
    if (name == "FoliageCullingCS")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL, 0);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL, 0);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, D3D12_SHADER_VISIBILITY_ALL, 0);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "FoliageCullingCS");
    }
    else if (name == "Generate3DNoiseCS")
    {
        RootSignatureBuilder builder;

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "Generate3DNoiseCS");
    }
    else if (name == "VolumetricFogBilateralCS")
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "VolumetricFogBilateralCS");
    }
    if (name == "VolumetricFogInjectionCS")
    {
        // --- CBV (定数バッファ) ---
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL); // [0] b0: FrameData
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_ALL); // [1] b2: VolumetricFogSettings
        builder.AddCBV(3, D3D12_SHADER_VISIBILITY_ALL); // [2] b3: PointLights
        builder.AddCBV(4, D3D12_SHADER_VISIBILITY_ALL); // [3] b4: SpotLights
        builder.AddCBV(5, D3D12_SHADER_VISIBILITY_ALL); // [4] b5: FogVolumeBuffer
        builder.AddCBV(6, D3D12_SHADER_VISIBILITY_ALL); // [5] b6: ShadowData

        // --- Descriptor Tables ---
        // [6] SRV: t0 ~ t2 (Depth, ShadowMap, NoiseVolume の3つ)
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);

        // [7] UAV: u0 (VoxelInject の1つ)
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        // --- Static Samplers ---
        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddStaticSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_ALL, D3D12_COMPARISON_FUNC_LESS_EQUAL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
        return builder.Build(device_, csFlags, "VolumetricFogInjectionCS");
    }
    if (name == "VoxelSpatialFilterCS")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
        return builder.Build(device_, csFlags, "VoxelSpatialFilterCS");
    }

    if (name == "VoxelTemporalResolveCS")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddStaticSampler(
            0,                                   
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,     
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,    
            D3D12_SHADER_VISIBILITY_ALL
        );

        D3D12_ROOT_SIGNATURE_FLAGS csFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
        return builder.Build(device_, csFlags, "VoxelTemporalResolveCS");
    }

    if (name == "VoxelIntegrateCS")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
        return builder.Build(device_, csFlags, "VoxelIntegrateCS");
    }
    if (name == "VolumetricFogResolveCS")
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(2, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
        return builder.Build(device_, csFlags, "VolumetricFogResolveCS");
    }
    
    else if (name == "FluidSimulationCS") 
    {
        RootSignatureBuilder builder;

        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        return builder.Build(device_, csFlags, "FluidComputeRS");
    }
    if (name == "WorldInteractionCS") 
    {
        builder.AddCBV(0, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddCBV(1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
        builder.AddDescriptorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);

        builder.AddStaticSampler(
            0,
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_SHADER_VISIBILITY_ALL
        );

        builder.AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS csFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
        return builder.Build(device_, csFlags, "WorldInteractionCS");
    }

    // どれにも該当しない
    LOG_ERROR("Unknown RootSignature: {}", name);
    assert(false && "Unknown RootSignature name.");
    return nullptr;
}

}