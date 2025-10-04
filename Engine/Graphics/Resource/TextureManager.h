#pragma once

#include "StringUtils.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/DirectXTex/DirectXTex.h"    
#include "BufferManager.h"
#include "CommandManager.h"
#include "SRVManager.h"
#include "GraphicDevice.h"
#include "SRVAllocator.h"

#include <d3d12.h>                
#include <wrl/client.h>           
#include <DirectXMath.h>             
#include <string>                 
#include <cassert>                
#include <vector>                 
#include <d3dcompiler.h> 

class TextureManager
{
public:
    struct TextureResources
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> texture;
        Microsoft::WRL::ComPtr<ID3D12Resource> intermediate;
        DirectX::TexMetadata metadata;
        SRVManager srvManager;
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU;
    };

    struct UploadResourceEntry {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint64_t fenceValue;
    };

    TextureManager();
    ~TextureManager();

    static DirectX::ScratchImage LoadTexture(const std::string& filePath);

    TextureResources CreateTexture2DArray(
        const std::vector<DirectX::ScratchImage>& mipImagesArray,
        ID3D12DescriptorHeap* srvDescriptorHeap,
        ID3D12Device* device,
        uint32_t descriptorSizeSRV,
        ID3D12GraphicsCommandList* commandList
    );

    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, SRVAllocator* srvAllocator);

    TextureResources UploadTexture(DirectX::ScratchImage& mipImages, ID3D12DescriptorHeap* srvDescriptorHeap,
        GraphicDevice& graphicDevice, uint32_t descriptorSizeSRV, std::vector<TextureResources>& textures_);

    TextureResources UploadTex(DirectX::ScratchImage& mipImages);

    void AddNewUpload(const TextureResources& resource) {
        newUploads_.push_back(resource);
    }

    const std::vector<TextureResources>& GetNewUploads() const {
        return newUploads_;
    }

    void ClearNewUploads() {
        newUploads_.clear();
    }


    void RegisterPendingUpload(Microsoft::WRL::ComPtr<ID3D12Resource> intermediate, uint64_t fenceValue);
    void CleanupCompletedUploads(uint64_t completedFenceValue);

    std::vector<DirectX::ScratchImage> LoadMultipleTextures(const std::vector<std::string>& texturePaths);
    void CreateAndUploadTexture2DArray(
        const std::vector<DirectX::ScratchImage>& images,
        ID3D12DescriptorHeap* srvHeap,
        uint32_t descriptorSize,
        TextureResources& outTextureArrayResource);
    

    // ゲッター
    const std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>& GetAllSRVs() const { return allSRVs_; }
    const std::vector<UploadResourceEntry>& GetPendingUploadResources() const { return pendingUploadResources_; }

    D3D12_GPU_DESCRIPTOR_HANDLE textureArraySRV_;           // SRVのGPUハンドル
private:
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);
    static Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

    uint32_t AllocateAndRegisterSRV(D3D12_GPU_DESCRIPTOR_HANDLE handle);

    ID3D12Device* device_ = nullptr;
    ID3D12GraphicsCommandList* commandList_ = nullptr;
    SRVAllocator* srvAllocator_;

    std::vector<UploadResourceEntry> pendingUploadResources_;
    std::vector<TextureResources> newUploads_;
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> allSRVs_;

};