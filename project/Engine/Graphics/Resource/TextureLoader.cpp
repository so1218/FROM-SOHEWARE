#include "pch.h"
#include "TextureLoader.h"
#include "StringUtils.h"
#include "BufferManager.h"
#include "SRVManager.h"

namespace FE
{

TextureLoader::TextureLoader() {};

TextureLoader::~TextureLoader()
{
}

void TextureLoader::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, SRVManager* srvManager)
{
    device_ = device;
    commandList_ = commandList;
    srvManager_ = srvManager;
}

DirectX::ScratchImage TextureLoader::LoadTexture(const std::string& filePath)
{
    DirectX::ScratchImage image{};
    std::wstring filePathW = StringUtils::ConvertString(filePath);
    HRESULT hr = E_FAIL;

    // DDSファイル優先（高速パス）
    if (filePathW.ends_with(L".dds"))
    {
        hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
    }
    else
    {
        // 開発・エディタ用のフォールバック（WICロード）
        hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);

        // 開発用：ミップマップが含まれていない場合は生成
        if (SUCCEEDED(hr) && image.GetMetadata().mipLevels == 1 && !DirectX::IsCompressed(image.GetMetadata().format))
        {
            DirectX::ScratchImage mipImages{};
            if (SUCCEEDED(DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages)))
            {
                image = std::move(mipImages);
            }
        }
    }

    assert(SUCCEEDED(hr) && "Failed to load texture file.");
    return image;
}

Microsoft::WRL::ComPtr<ID3D12Resource> TextureLoader::CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata)
{
    // metadataを基にResourceの設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = uint32_t(metadata.width);// Textureの幅
    resourceDesc.Height = uint32_t(metadata.height);// Textureの高さ
    resourceDesc.MipLevels = uint16_t(metadata.mipLevels);// mipmapの数
    resourceDesc.DepthOrArraySize = uint16_t(metadata.arraySize);// 奥行き or 配列Textureの配列数
    resourceDesc.Format = metadata.format;// TextureのFormat
    resourceDesc.SampleDesc.Count = 1;// サンプリングカウント。1固定
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);// Textureの次元数。普段使っているのは2次元

    // 利用するHeapの設定
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// 細かい設定を行う
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;// 無効
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;// 無効

    // Resourceの生成
    Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,// Heapの設定
        D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定
        &resourceDesc,// Resourceの設定
        D3D12_RESOURCE_STATE_COPY_DEST,// データ転送される設定
        nullptr,// 使わないのでnullptr
        IID_PPV_ARGS(&resource));// 作成するResourceポインタへのポインタ
    assert(SUCCEEDED(hr));
    resource->SetName(L"TextureResource");

    return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> TextureLoader::UploadTextureData(
    ID3D12Resource* texture,
    const DirectX::ScratchImage& mipImages,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList)
{
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);

    uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, uint32_t(subresources.size()));

    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = BufferManager::CreateBufferResource(device, intermediateSize);
    intermediateResource->SetName(L"TextureUploadIntermediate");

    UpdateSubresources(commandList, texture, intermediateResource.Get(), 0, 0, uint32_t(subresources.size()), subresources.data());

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    commandList->ResourceBarrier(1, &barrier);

    return intermediateResource;
}

TextureLoader::TextureResources TextureLoader::UploadTexture(DirectX::ScratchImage& mipImages)
{
    TextureResources result;

    result.metadata = mipImages.GetMetadata();
    result.texture = CreateTextureResource(device_, result.metadata);

    // 中間バッファを作成してデータを転送
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediate = UploadTextureData(result.texture.Get(), mipImages, device_, commandList_);

    // SRVの作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = result.metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (result.metadata.IsCubemap())
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = uint32_t(result.metadata.mipLevels);
    }
    else
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = uint32_t(result.metadata.mipLevels);
    }

    result.srvIndex = srvManager_->CreateSRV(result.texture.Get(), srvDesc);

    // 中間バッファはTextureResourcesには保持させず、後でフェンス同期で自動解放
    result.intermediate = intermediate; // トランジション完了までは一時保持
    loadedTextures_.push_back(result);

    return result;
}

TextureLoader::TextureResources TextureLoader::UploadTex(DirectX::ScratchImage& mipImages)
{
    TextureResources result;

    result.metadata = mipImages.GetMetadata();
    result.texture = CreateTextureResource(device_, result.metadata);
    result.intermediate = UploadTextureData(result.texture.Get(), mipImages, device_, commandList_);

    return result;
}

void TextureLoader::RegisterPendingUpload(Microsoft::WRL::ComPtr<ID3D12Resource> intermediate, uint64_t fenceValue)
{
    pendingUploadResources_.push_back({ intermediate, fenceValue });
}

void TextureLoader::CleanupCompletedUploads(uint64_t completedFenceValue)
{
    size_t before = pendingUploadResources_.size();
    auto it = pendingUploadResources_.begin();
    while (it != pendingUploadResources_.end()) {
        if (it->fenceValue <= completedFenceValue) {
            it = pendingUploadResources_.erase(it);
        }
        else {
            ++it;
        }
    }
    size_t after = pendingUploadResources_.size();
}

std::vector<DirectX::ScratchImage> TextureLoader::LoadMultipleTextures(const std::vector<std::string>& texturePaths)
{
    std::vector<DirectX::ScratchImage> images;
    for (const auto& path : texturePaths)
    {
        DirectX::ScratchImage img = TextureLoader::LoadTexture(path);
        images.push_back(std::move(img));
    }
    return images;
}

uint32_t TextureLoader::Load(const std::string& filePath)
{
    // キャッシュを検索(すでに同じパスのテクスチャがロードされているか)
    auto it = textureCache_.find(filePath);
    if (it != textureCache_.end())
    {
        // 既にロード済みなら、保存されているSRVインデックスをそのまま返す
        return it->second;
    }

    // まだロードされていなければ、画像を読み込む 
    DirectX::ScratchImage mipImages = LoadTexture(filePath);

    // GPUへアップロード＆SRV作成 
    TextureResources texResources = UploadTexture(mipImages);

    // 次回のためにキャッシュに登録
    textureCache_[filePath] = texResources.srvIndex;

    // SRVインデックスを返す
    return texResources.srvIndex;
}

}