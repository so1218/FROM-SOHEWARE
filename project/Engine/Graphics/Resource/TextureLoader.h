#pragma once

#include "StringUtils.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/DirectXTex/DirectXTex.h"    
#include "BufferManager.h"
#include "CommandManager.h"
#include "SRVManager.h"
#include "GraphicsDevice.h"

#include <d3d12.h>                
#include <wrl/client.h>           
#include <DirectXMath.h>             
#include <string>                 
#include <cassert>                
#include <vector>                 
#include <d3dcompiler.h> 

class TextureLoader
{
public:
    struct TextureResources
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> texture;
        Microsoft::WRL::ComPtr<ID3D12Resource> intermediate;
        DirectX::TexMetadata metadata;
        uint32_t srvIndex; // SRVヒープのインデックス番号を保存
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU;
    };

    struct UploadResourceEntry 
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint64_t fenceValue;
    };

    TextureLoader();
    ~TextureLoader();

    static DirectX::ScratchImage LoadTexture(const std::string& filePath);

    TextureResources CreateTexture2DArray(
        const std::vector<DirectX::ScratchImage>& mipImagesArray
    );

    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, SRVManager* srvManager);

    TextureResources UploadTexture(DirectX::ScratchImage& mipImages);

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
        TextureResources& outTextureArrayResource);
    

    // ゲッター
    const std::vector<UploadResourceEntry>& GetPendingUploadResources() const { return pendingUploadResources_; }

private:
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);
    static Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

    ID3D12Device* device_ = nullptr;
    ID3D12GraphicsCommandList* commandList_ = nullptr;

    std::vector<UploadResourceEntry> pendingUploadResources_;
    SRVManager* srvManager_;

    uint32_t textureArraySrvIndex_;

    // 永続リスト (デストラクタでのSRV解放用)
    std::vector<TextureResources> loadedTextures_;

    // 一時リスト (中間バッファのクリーンアップ用)
    std::vector<TextureResources> newUploads_;
};