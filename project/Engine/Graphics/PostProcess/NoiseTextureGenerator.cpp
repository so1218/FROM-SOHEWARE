#include "pch.h"
#include "NoiseTextureGenerator.h"
#include "Engine.h"

namespace FE
{

void NoiseTextureGenerator::Initialize(Engine* engine)
{
    engine_ = engine;
}

GeneratedTextureData NoiseTextureGenerator::Generate3DPerlinNoise(
    ID3D12GraphicsCommandList* cmdList,
    UINT width, UINT height, UINT depth)
{
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    SRVManager* srvManager = engine_->GetSRVManager();

    GeneratedTextureData result;

    // 3Dテクスチャリソースの作成
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT; 

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    resDesc.Width = width;
    resDesc.Height = height;
    resDesc.DepthOrArraySize = depth;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; 
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // 最初は書き込み状態
        nullptr,
        IID_PPV_ARGS(&result.resource)
    );
    result.resource->SetName(L"3D_PerlinNoise_Texture");

    // UAV と SRV を作成

    // UAV (書き込み用)
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = resDesc.Format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    uavDesc.Texture3D.MipSlice = 0;
    uavDesc.Texture3D.FirstWSlice = 0;
    uavDesc.Texture3D.WSize = depth;
    uint32_t uavIndex = srvManager->CreateUAV(result.resource.Get(), uavDesc);

    // SRV (読み込み用)
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = resDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Texture3D.MipLevels = 1;
    srvDesc.Texture3D.MostDetailedMip = 0;
    result.srvIndex = srvManager->CreateSRV(result.resource.Get(), srvDesc);

    // Compute Shader で書き込み 

    // ノイズ生成用のPSOとRootSignatureをセット
    cmdList->SetComputeRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("Generate3DNoiseCS"));
    cmdList->SetPipelineState(engine_->GetPSOManager()->GetPSO("Generate3DNoise.CS"));

    // SRVManagerのヒープをセット
    ID3D12DescriptorHeap* heaps[] = { srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // UAVをRootParameterにセット (GPUハンドルを取得して渡す)
    cmdList->SetComputeRootDescriptorTable(0, srvManager->GetSRVHandleGPU(uavIndex));

    // Dispatch (8x8x8 スレッドで処理)
    UINT dispatchX = (width + 7) / 8;
    UINT dispatchY = (height + 7) / 8;
    UINT dispatchZ = (depth + 7) / 8;
    cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

    // リソースバリア (UAV -> SRV)
    // 書き込みが終わったので、フォグから読み込めるように状態遷移
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = result.resource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    return result;
}

}