#include "SRVManager.h"
#include "DescriptorHeapManager.h"
#include "Logger.h"

D3D12_GPU_DESCRIPTOR_HANDLE SRVManager::Create(
    const Microsoft::WRL::ComPtr <ID3D12Resource>& textureResource,
    const DirectX::TexMetadata& metadata,
    ID3D12DescriptorHeap* descriptorHeap,
    const Microsoft::WRL::ComPtr <ID3D12Device>& device,
    uint32_t descriptorSizeSRV,
    uint32_t index
) 
{
    LOG_INFO("\n"
        "//=========================================================\n"
        "// Creating Shader Resource View (SRV)\n"
        "// Index: {}", index);

    // metaDataを基にSRVの設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);


    // SRVを作成するDescriptorHeapの場所を決める
    srvHandleCPU_ = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandleGPU_ = descriptorHeap->GetGPUDescriptorHandleForHeapStart();

    // 先頭はImGuiが使っているのでその次を使う
    srvHandleCPU_.ptr += descriptorSizeSRV * index;
    srvHandleGPU_.ptr += descriptorSizeSRV * index;

    LOG_DEBUG("  -> SRV CPU Handle: {:#x}", srvHandleCPU_.ptr);
    LOG_DEBUG("  -> SRV GPU Handle: {:#x}", srvHandleGPU_.ptr);

    // SRVの生成
    device->CreateShaderResourceView(textureResource.Get(), &srvDesc, srvHandleCPU_);

    LOG_INFO("// SRV Creation Successful.\n"
        "//=========================================================");
    return srvHandleGPU_;
}

D3D12_GPU_DESCRIPTOR_HANDLE SRVManager::CreateTexture2DArraySRV(
    const Microsoft::WRL::ComPtr<ID3D12Resource>& textureResource,
    const DirectX::TexMetadata& metadata,
    ID3D12DescriptorHeap* descriptorHeap,
    const Microsoft::WRL::ComPtr<ID3D12Device>& device,
    uint32_t descriptorSizeSRV,
    uint32_t index
) {
    LOG_INFO("\n"
        "//=========================================================\n"
        "// Creating Texture2DArray SRV\n"
        "// Index: {}, ArraySize: {}", index, metadata.arraySize);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = metadata.format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = static_cast<UINT>(metadata.mipLevels);
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = static_cast<UINT>(metadata.arraySize);

    // デスクリプタ位置計算
    srvHandleCPU_ = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandleGPU_ = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    srvHandleCPU_.ptr += descriptorSizeSRV * index;
    srvHandleGPU_.ptr += descriptorSizeSRV * index;

    LOG_DEBUG("  -> SRV CPU Handle: {:#x}", srvHandleCPU_.ptr);
    LOG_DEBUG("  -> SRV GPU Handle: {:#x}", srvHandleGPU_.ptr);

    // SRV作成
    device->CreateShaderResourceView(textureResource.Get(), &srvDesc, srvHandleCPU_);

    LOG_INFO("// Texture2DArray SRV Creation Successful.\n"
        "//=========================================================");

    return srvHandleGPU_;
}
