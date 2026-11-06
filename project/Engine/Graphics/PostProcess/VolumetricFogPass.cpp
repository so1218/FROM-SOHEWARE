#include "pch.h"
#include "VolumetricFogPass.h"
#include "Engine.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "GlobalConstants.h"
#include "PSOManager.h"
#include "EnvironmentManager.h"
#include "PIXColors.h"

namespace FS
{

void VolumetricFogPass::Initialize(Engine* engine, uint32_t width, uint32_t height, PSOManager* psoManager)
{
    InitializeBase(engine, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, true);
    psoManager_ = psoManager;

    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();

    // 保存した値が無いときの初期値
    constantBuffer_ = BufferManager::CreateMappedConstantBuffer(device, &settings_);
    settings_->albedo = { 0.8f, 0.8f, 0.8f };
    settings_->extinctionScale = 0.2f;
    settings_->anisotropy = 0.7f;
    settings_->extinction = 0.005f;
    settings_->heightDensity = 0.0f;
    settings_->baseHeight = 0.0f;
    settings_->heightFalloff = 0.1f;
    settings_->ambientLight = { 0.0f, 0.0f, 0.0f };
    settings_->temporalWeight = 0.05f;
    settings_->maxDistance = 500.0f;
    settings_->noiseScale = 0.01f;
    settings_->noiseDistortion = 0.15f;
    settings_->windSpeedMultiplier = 1.0f;
    settings_->coverage = 0.85f;
    settings_->erosion = 0.4f;
    settings_->noiseFeather = 0.3f;
    settings_->noiseIntensity = 1.0f;

    volumeConstantBuffer_ = BufferManager::CreateMappedConstantBuffer(device, &volumeBuffer_);
    memset(volumeBuffer_, 0, sizeof(FogVolumeBuffer));

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 16;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));
    passHeap_->SetName(L"VolumetricFog_Heap");

    injectTexture_ = CreateFroxelTexture(device, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"VoxelInject");
    filteredTexture_ = CreateFroxelTexture(device, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"VoxelInjectFiltered");
    accumulateTexture_ = CreateFroxelTexture(device, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"VoxelAccumulate");
    historyTextures_[0] = CreateFroxelTexture(device, D3D12_RESOURCE_STATE_COMMON, L"VoxelHistory_0");
    historyTextures_[1] = CreateFroxelTexture(device, D3D12_RESOURCE_STATE_COMMON, L"VoxelHistory_1");

    // 生成のコマンドは初期化用のコマンドリストに積み、ロードのコマンドと一緒に実行される
    NoiseTextureGenerator noiseGenerator;
    noiseGenerator.Initialize(engine);
    noiseTexture_ = noiseGenerator.Generate3DPerlinNoise(engine->GetCommandManager()->GetCommandList(),
        kNoiseResolution, kNoiseResolution, kNoiseResolution);

    // 画面に戻した結果は、InitializeBase が作った画面の半分の大きさのテクスチャに書く
    textureResource_->SetName(L"VolumetricFog_ResolveOutput");
}

VolumetricFogPass::FroxelTexture VolumetricFogPass::CreateFroxelTexture(ID3D12Device* device, D3D12_RESOURCE_STATES initialState, const wchar_t* name)
{
    FroxelTexture texture;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex3D(
        DXGI_FORMAT_R16G16B16A16_FLOAT, kFroxelWidth, kFroxelHeight, kFroxelDepth, 1,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, initialState, nullptr, IID_PPV_ARGS(&texture.resource));
    texture.resource->SetName(name);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    uavDesc.Texture3D.WSize = kFroxelDepth;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture3D.MipLevels = 1;

    auto* srvManager = engine_->GetSRVManager();
    texture.uavIndex = srvManager->Allocate();
    device->CreateUnorderedAccessView(texture.resource.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(texture.uavIndex));
    texture.srvIndex = srvManager->Allocate();
    device->CreateShaderResourceView(texture.resource.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(texture.srvIndex));

    return texture;
}

void VolumetricFogPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context, D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    PIXScopedEvent(cmdList, PIXColors::Compute, "Volumetric Fog Pass");

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    SRVManager* srvManager = engine_->GetSRVManager();
    const uint32_t handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const D3D12_CPU_DESCRIPTOR_HANDLE heapStartCPU = passHeap_->GetCPUDescriptorHandleForHeapStart();
    const D3D12_GPU_DESCRIPTOR_HANDLE heapStartGPU = passHeap_->GetGPUDescriptorHandleForHeapStart();

    auto CopyToHeap = [&](uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE source)
        {
            device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(heapStartCPU, slot, handleSize), source, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        };
    auto HeapGPU = [&](uint32_t slot)
        {
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStartGPU, slot, handleSize);
        };
    auto BindCommonConstants = [&](const char* name)
        {
            cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature(name));
            cmdList->SetPipelineState(psoManager_->GetPSO(name));
            cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());
            cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress());
        };
    auto ToShaderResource = [&](ID3D12Resource* resource)
        {
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            cmdList->ResourceBarrier(1, &barrier);
        };

    // 結果は次のコンピュート (ぼかし) と合成のピクセルシェーダーの両方で読む
    constexpr D3D12_RESOURCE_STATES kOutputReadState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    const bool isFirstFrame = (frameCounter_ == 0);
    const uint32_t currentHistory = frameCounter_ % 2;
    const uint32_t previousHistory = (frameCounter_ + 1) % 2;

    // 影はピクセルシェーダー用の状態なので、コンピュートでも読めるようにする
    // 初回だけはまだ一度も書いていないので COMMON から
    D3D12_RESOURCE_BARRIER beginBarriers[3] = {
        CD3DX12_RESOURCE_BARRIER::Transition(engine_->GetShadowMap()->GetResource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(historyTextures_[previousHistory].resource.Get(),
            isFirstFrame ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(textureResource_.Get(),
            isFirstFrame ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : kOutputReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
    };
    cmdList->ResourceBarrier(_countof(beginBarriers), beginBarriers);

    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    const uint32_t groupCountX = (kFroxelWidth + 7) / 8;
    const uint32_t groupCountY = (kFroxelHeight + 7) / 8;
    const uint32_t groupCountZ = (kFroxelDepth + 3) / 4;

    // 1. 各フロクセルに光と濃さを入れる
    {
        PIXScopedEvent(cmdList, PIXColors::Compute, "Injection");

        CopyToHeap(0, context.GetCPUHandle(context.sceneDepthSrvIndex));
        CopyToHeap(1, engine_->GetShadowMap()->GetSRVHandleCPU());
        CopyToHeap(2, srvManager->GetSRVHandleCPU_ForCopying(noiseTexture_.srvIndex));
        CopyToHeap(3, srvManager->GetSRVHandleCPU_ForCopying(injectTexture_.uavIndex));

        BindCommonConstants("VolumetricFogInjectionCS");
        cmdList->SetComputeRootConstantBufferView(2, engine_->GetLightManager()->GetPointLightResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(3, engine_->GetLightManager()->GetSpotLightResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(4, volumeConstantBuffer_->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(5, engine_->GetLightManager()->GetShadowDataResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(6, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootDescriptorTable(7, HeapGPU(0));
        cmdList->SetComputeRootDescriptorTable(8, HeapGPU(3));
        cmdList->Dispatch(groupCountX, groupCountY, groupCountZ);

        ToShaderResource(injectTexture_.resource.Get());
    }

    // 2. となりのフロクセルとならす
    {
        PIXScopedEvent(cmdList, PIXColors::Compute, "Spatial Filter");

        CopyToHeap(4, srvManager->GetSRVHandleCPU_ForCopying(injectTexture_.srvIndex));
        CopyToHeap(5, srvManager->GetSRVHandleCPU_ForCopying(filteredTexture_.uavIndex));

        BindCommonConstants("VoxelSpatialFilterCS");
        cmdList->SetComputeRootDescriptorTable(2, HeapGPU(4));
        cmdList->SetComputeRootDescriptorTable(3, HeapGPU(5));
        cmdList->Dispatch(groupCountX, groupCountY, groupCountZ);

        ToShaderResource(filteredTexture_.resource.Get());
    }

    // 3. 前のフレームの結果と混ぜる
    {
        PIXScopedEvent(cmdList, PIXColors::Compute, "Temporal Resolve");

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(historyTextures_[currentHistory].resource.Get(),
            isFirstFrame ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &barrier);

        CopyToHeap(6, srvManager->GetSRVHandleCPU_ForCopying(filteredTexture_.srvIndex));
        CopyToHeap(7, srvManager->GetSRVHandleCPU_ForCopying(historyTextures_[previousHistory].srvIndex));
        CopyToHeap(8, srvManager->GetSRVHandleCPU_ForCopying(historyTextures_[currentHistory].uavIndex));

        BindCommonConstants("VoxelTemporalResolveCS");
        cmdList->SetComputeRootDescriptorTable(2, HeapGPU(6));
        cmdList->SetComputeRootDescriptorTable(3, HeapGPU(8));
        cmdList->Dispatch(groupCountX, groupCountY, groupCountZ);

        ToShaderResource(historyTextures_[currentHistory].resource.Get());
    }

    // 4. 手前から奥へ積み上げる。奥行きはシェーダーの中で回すので Z は 1
    {
        PIXScopedEvent(cmdList, PIXColors::Compute, "Integrate");

        CopyToHeap(9, srvManager->GetSRVHandleCPU_ForCopying(historyTextures_[currentHistory].srvIndex));
        CopyToHeap(10, srvManager->GetSRVHandleCPU_ForCopying(accumulateTexture_.uavIndex));

        BindCommonConstants("VoxelIntegrateCS");
        cmdList->SetComputeRootDescriptorTable(2, HeapGPU(9));
        cmdList->SetComputeRootDescriptorTable(3, HeapGPU(10));
        cmdList->Dispatch(groupCountX, groupCountY, 1);

        ToShaderResource(accumulateTexture_.resource.Get());
    }

    // 5. 画面の各ピクセルで、手前の物までの霧を読む
    {
        PIXScopedEvent(cmdList, PIXColors::Compute, "Resolve 2D");

        CopyToHeap(11, context.GetCPUHandle(context.sceneDepthSrvIndex));
        CopyToHeap(12, srvManager->GetSRVHandleCPU_ForCopying(accumulateTexture_.srvIndex));
        CopyToHeap(13, srvManager->GetSRVHandleCPU_ForCopying(uavIndex_));

        BindCommonConstants("VolumetricFogResolveCS");
        cmdList->SetComputeRootDescriptorTable(2, HeapGPU(11));
        cmdList->SetComputeRootDescriptorTable(3, HeapGPU(13));

        // 出力は画面の半分の大きさなので、画面全体の数で Dispatch しない
        const uint32_t outputWidth = static_cast<uint32_t>(viewport_.Width);
        const uint32_t outputHeight = static_cast<uint32_t>(viewport_.Height);
        cmdList->Dispatch((outputWidth + 7) / 8, (outputHeight + 7) / 8, 1);
    }

    // 次のフレームでまた書き込めるように戻す。影はピクセルシェーダー用に戻す
    D3D12_RESOURCE_BARRIER endBarriers[6] = {
        CD3DX12_RESOURCE_BARRIER::Transition(injectTexture_.resource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(filteredTexture_.resource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(accumulateTexture_.resource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(historyTextures_[currentHistory].resource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(engine_->GetShadowMap()->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(textureResource_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kOutputReadState),
    };
    cmdList->ResourceBarrier(_countof(endBarriers), endBarriers);

    frameCounter_++;
}

void VolumetricFogPass::BeginFrame()
{
    if (volumeBuffer_)
    {
        volumeBuffer_->volumeCount = 0;
    }
}

bool VolumetricFogPass::SubmitFogVolume(const FogVolumeData& volumeData)
{
    if (!volumeBuffer_ || !volumeData.isVisible || volumeBuffer_->volumeCount >= static_cast<uint32_t>(MAX_FOG_VOLUMES))
    {
        return false;
    }

    // 球は x の大きさだけを使う
    Vector3 scale = (volumeData.type == 0)
        ? Vector3{ volumeData.scale.x, volumeData.scale.x, volumeData.scale.x }
        : volumeData.scale;

    Matrix4x4 localToWorld =
        Matrix4x4::MakeScale(scale) *
        Matrix4x4::MakeRotateXYZ({
            Math::ToRadians(volumeData.rotation.x),
            Math::ToRadians(volumeData.rotation.y),
            Math::ToRadians(volumeData.rotation.z) }) *
        Matrix4x4::MakeTranslate(volumeData.position);

    FogVolume& volume = volumeBuffer_->volumes[volumeBuffer_->volumeCount];
    volume.worldToLocal = Matrix4x4::Inverse(localToWorld);
    volume.type = volumeData.type;
    volume.color = volumeData.color;
    volume.density = volumeData.density;
    volume.noiseScale = volumeData.noiseScale;
    volume.noiseIntensity = volumeData.noiseIntensity;
    volume.windDirection = volumeData.windDirection;
    volume.windSpeed = volumeData.windSpeed;
    volume.blendDistance = volumeData.blendDistance;
    volume.coverage = volumeData.coverage;
    volume.erosion = volumeData.erosion;
    volume.noiseFeather = volumeData.noiseFeather;
    volume.distortionAmount = volumeData.distortionAmount;
    volume.heightFalloff = volumeData.heightFalloff;

    volumeBuffer_->volumeCount++;
    return true;
}

}
