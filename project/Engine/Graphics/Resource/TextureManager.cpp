#include "TextureManager.h"

TextureManager::TextureManager() {};

TextureManager::~TextureManager() {}

void TextureManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, SRVManager* srvManager)
{
    device_ = device;
    commandList_ = commandList;
    srvManager_ = srvManager;
}

DirectX::ScratchImage TextureManager::LoadTexture(const std::string& filePath)
{
    // テクスチャファイルを読み込み、プログラムで扱える形式に変換
    DirectX::ScratchImage image{};
    std::wstring filePathW = StringUtils::ConvertString(filePath);
    HRESULT hr;

    // 拡張子によって読み込み方法を切り替え
    if (filePathW.ends_with(L".dds"))
    {
        hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
    }
    else
    {
        hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
    }
    assert(SUCCEEDED(hr));

    // ミップマップを生成（圧縮フォーマットの場合はスキップ）
    DirectX::ScratchImage mipImages{};
    if (DirectX::IsCompressed(image.GetMetadata().format))
    {
        mipImages = std::move(image);
    }
    else
    {
        hr = DirectX::GenerateMipMaps(
            image.GetImages(),
            image.GetImageCount(),
            image.GetMetadata(),
            DirectX::TEX_FILTER_SRGB,
            0,
            mipImages
        );
        assert(SUCCEEDED(hr));
    }

    // ミップマップ付きテクスチャを返す
    return mipImages;
}

TextureManager::TextureResources TextureManager::CreateTexture2DArray(
    const std::vector<DirectX::ScratchImage>& mipImagesArray
) {
    TextureResources result;

    assert(!mipImagesArray.empty());

    const auto& baseMeta = mipImagesArray[0].GetMetadata();
    size_t arraySize = mipImagesArray.size();

    // Texture2DArray Resource 作成
    DirectX::TexMetadata arrayMeta = baseMeta;
    arrayMeta.arraySize = static_cast<size_t>(arraySize);
    result.metadata = arrayMeta;

    D3D12_RESOURCE_DESC desc{};
    desc.Width = UINT(arrayMeta.width);
    desc.Height = UINT(arrayMeta.height);
    desc.MipLevels = UINT16(arrayMeta.mipLevels);
    desc.DepthOrArraySize = UINT16(arrayMeta.arraySize);
    desc.Format = arrayMeta.format;
    desc.SampleDesc.Count = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&result.texture)
    );
    assert(SUCCEEDED(hr));

    // Upload
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;

    for (size_t i = 0; i < arraySize; ++i) {
        std::vector<D3D12_SUBRESOURCE_DATA> subresourceTmp;
        DirectX::PrepareUpload(device_, mipImagesArray[i].GetImages(), mipImagesArray[i].GetImageCount(), mipImagesArray[i].GetMetadata(), subresourceTmp);
        subresources.insert(subresources.end(), subresourceTmp.begin(), subresourceTmp.end());
    }

    UINT64 requiredSize = GetRequiredIntermediateSize(result.texture.Get(), 0, static_cast<UINT>(subresources.size()));
    result.intermediate = BufferManager::CreateBufferResource(device_, requiredSize);

    UpdateSubresources(commandList_, result.texture.Get(), result.intermediate.Get(), 0, 0, static_cast<UINT>(subresources.size()), subresources.data());

    // Resource Barrier
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = result.texture.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &barrier);

    // SRVインデックスは「カラ」のまま返す
    result.srvIndex = 0;

    return result;
}
 
Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata)
{
    // 1.metadataを基にResourceの設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = UINT(metadata.width);// Textureの幅
    resourceDesc.Height = UINT(metadata.height);// Textureの高さ
    resourceDesc.MipLevels = UINT16(metadata.mipLevels);// mipmapの数
    resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);// 奥行き or 配列Textureの配列数
    resourceDesc.Format = metadata.format;// TextureのFormat
    resourceDesc.SampleDesc.Count = 1;// サンプリングカウント。1固定
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);// Textureの次元数。普段使っているのは2次元

    // 2.利用するHeapの設定。非常に特殊な運用。02_04exで一般的なケース版がある
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// 細かい設定を行う
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;// 無効
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;// 無効

    // 3.Resourceの生成
    Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,// Heapの設定
        D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特になし
        &resourceDesc,// Resourceの設定
        D3D12_RESOURCE_STATE_COPY_DEST,// データ転送される設定
        nullptr,// Clear最適値。使わないのでnullptr
        IID_PPV_ARGS(&resource));// 作成するResourceポインタへのポインタ
    assert(SUCCEEDED(hr));
    resource->SetName(L"TextureResource");

    return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList)
{
    std::vector<D3D12_SUBRESOURCE_DATA>subresources;
    DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
    uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
    Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = BufferManager::CreateBufferResource(device, intermediateSize);
    intermediateResource->SetName(L"TextureUploadIntermediate");
    UpdateSubresources(commandList, texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
    // Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
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

TextureManager::TextureResources TextureManager::UploadTexture(
    DirectX::ScratchImage& mipImages,
    std::vector<TextureResources>& textures)
{
    TextureResources result;

    // 1. テクスチャリソースとアップロード用バッファを作成
    result.metadata = mipImages.GetMetadata();
    result.texture = CreateTextureResource(device_, result.metadata);
    result.intermediate = UploadTextureData(result.texture.Get(), mipImages, device_, commandList_);

    // 2. シェーダーリソースビュー（SRV）の設定を構築
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = result.metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = UINT(result.metadata.mipLevels);

    // 3. SRVManagerを通してSRVを作成し、インデックスを取得
    result.srvIndex = srvManager_->CreateSRV(result.texture.Get(), srvDesc);

    // 4. テクスチャリストに追加し、新規アップロードとして登録
    textures.push_back(result);
    AddNewUpload(result);

    return result;
}

TextureManager::TextureResources TextureManager::UploadTex(DirectX::ScratchImage& mipImages)
{
    TextureResources result;

    result.metadata = mipImages.GetMetadata();
    result.texture = CreateTextureResource(device_, result.metadata);
    result.intermediate = UploadTextureData(result.texture.Get(), mipImages, device_, commandList_);

    return result;
}

void TextureManager::RegisterPendingUpload(Microsoft::WRL::ComPtr<ID3D12Resource> intermediate, uint64_t fenceValue)
{
    pendingUploadResources_.push_back({ intermediate, fenceValue });
}

void TextureManager::CleanupCompletedUploads(uint64_t completedFenceValue)
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

std::vector<DirectX::ScratchImage> TextureManager::LoadMultipleTextures(const std::vector<std::string>& texturePaths)
{
    std::vector<DirectX::ScratchImage> images;
    for (const auto& path : texturePaths)
    {
        DirectX::ScratchImage img = TextureManager::LoadTexture(path);
        images.push_back(std::move(img));
    }
    return images;
}

void TextureManager::CreateAndUploadTexture2DArray(
    const std::vector<DirectX::ScratchImage>& images,
    TextureResources& outTextureArrayResource)
{
    // 1. Texture2DArrayリソースを作成してGPUにアップロード
    outTextureArrayResource = CreateTexture2DArray(images);

    // 2. SRVの設定を構築
    const auto& arrayMeta = outTextureArrayResource.metadata;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = arrayMeta.format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MipLevels = UINT(arrayMeta.mipLevels);
    srvDesc.Texture2DArray.ArraySize = UINT(arrayMeta.arraySize);
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // 3. SRVManagerを使用してSRVを作成し、インデックスを取得
    uint32_t index = srvManager_->CreateSRV(outTextureArrayResource.texture.Get(), srvDesc);

    // 4. 取得したインデックスをリソース情報として保存
    outTextureArrayResource.srvIndex = index;
    textureArraySrvIndex_ = index;
}