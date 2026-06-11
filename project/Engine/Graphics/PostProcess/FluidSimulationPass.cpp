#include "pch.h"
#include "FluidSimulationPass.h"
#include "Engine.h"
#include "PSOManager.h"

namespace FE
{

FluidSimulationPass::~FluidSimulationPass()
{
    auto* srvManager = engine_->GetSRVManager();
    if (!srvManager) return;

    // 確保したすべてのSRV/UAVインデックスを解放
    for (int i = 0; i < 2; ++i) {
        if (velocitySrvIndices_[i] != 0) srvManager->FreeSRV(velocitySrvIndices_[i]);
        if (velocityUavIndices_[i] != 0) srvManager->FreeSRV(velocityUavIndices_[i]);
        if (densitySrvIndices_[i] != 0)  srvManager->FreeSRV(densitySrvIndices_[i]);
        if (densityUavIndices_[i] != 0)  srvManager->FreeSRV(densityUavIndices_[i]);
        if (pressureSrvIndices_[i] != 0) srvManager->FreeSRV(pressureSrvIndices_[i]);
        if (pressureUavIndices_[i] != 0) srvManager->FreeSRV(pressureUavIndices_[i]);
    }
    if (divergenceSrvIndex_ != 0) srvManager->FreeSRV(divergenceSrvIndex_);
    if (divergenceUavIndex_ != 0) srvManager->FreeSRV(divergenceUavIndex_);
}

void FluidSimulationPass::Initialize(Engine* engine, PSOManager* psoManager, UINT gridWidth, UINT gridHeight, UINT gridDepth)
{
    engine_ = engine;
    psoManager_ = psoManager;
    width_ = gridWidth;
    height_ = gridHeight;
    depth_ = gridDepth;

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();

    // 1. 定数バッファ (b1: FluidSettings) の作成とマップ
    constantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(FluidSettings));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));
    memset(cbData_, 0, sizeof(FluidSettings));

    // 流体設定の初期パラメータをセット
    cbData_->velocityDissipation = 0.995f; // 速度の減衰（少しずつ収まる）
    cbData_->densityDissipation = 0.990f; // 密度の減衰（霧が少しずつ消える）
    cbData_->gridScale = 1.0f;
    cbData_->interactionRadius = 4.0f;
    cbData_->injectionStrength = 10.0f;
    cbData_->densityAmount = 1.0f;

    // 2. 各種 3D テクスチャの生成
    // 速度ベクトル (xyzに速度が入るので、RGBAのFP16を使用)
    CreateFluidTexture3D(device, DXGI_FORMAT_R16G16B16A16_FLOAT, L"Fluid_Velocity_0", velocityRes_[0], velocityUavIndices_[0], velocitySrvIndices_[0]);
    CreateFluidTexture3D(device, DXGI_FORMAT_R16G16B16A16_FLOAT, L"Fluid_Velocity_1", velocityRes_[1], velocityUavIndices_[1], velocitySrvIndices_[1]);

    // 密度・圧力・発散 (すべてスカラー値なので、RのFP16で十分軽量かつ高精度)
    CreateFluidTexture3D(device, DXGI_FORMAT_R16_FLOAT, L"Fluid_Density_0", densityRes_[0], densityUavIndices_[0], densitySrvIndices_[0]);
    CreateFluidTexture3D(device, DXGI_FORMAT_R16_FLOAT, L"Fluid_Density_1", densityRes_[1], densityUavIndices_[1], densitySrvIndices_[1]);

    CreateFluidTexture3D(device, DXGI_FORMAT_R16_FLOAT, L"Fluid_Pressure_0", pressureRes_[0], pressureUavIndices_[0], pressureSrvIndices_[0]);
    CreateFluidTexture3D(device, DXGI_FORMAT_R16_FLOAT, L"Fluid_Pressure_1", pressureRes_[1], pressureUavIndices_[1], pressureSrvIndices_[1]);

    // Divergence は前フレームの記憶が不要なので1枚だけ
    CreateFluidTexture3D(device, DXGI_FORMAT_R16_FLOAT, L"Fluid_Divergence", divergenceRes_, divergenceUavIndex_, divergenceSrvIndex_);
}

// 3Dテクスチャ生成およびディスクリプタ登録の共通ヘルパー
void FluidSimulationPass::CreateFluidTexture3D(
    ID3D12Device* device,
    DXGI_FORMAT format,
    const wchar_t* debugName,
    Microsoft::WRL::ComPtr<ID3D12Resource>& outResource,
    uint32_t& outUavIndex,
    uint32_t& outSrvIndex)
{
    auto* srvManager = engine_->GetSRVManager();

    // 3Dテクスチャ設定
    CD3DX12_RESOURCE_DESC tex3DDesc = CD3DX12_RESOURCE_DESC::Tex3D(
        format, width_, height_, depth_, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    // リソース作成 (初期状態は他パスからの読み込みを考慮して SRV RESOURCE にしておく)
    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &tex3DDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr, IID_PPV_ARGS(&outResource));
    assert(SUCCEEDED(hr));
    outResource->SetName(debugName);

    // UAV ビュー作成
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    uavDesc.Texture3D.MipSlice = 0;
    uavDesc.Texture3D.FirstWSlice = 0;
    uavDesc.Texture3D.WSize = depth_;

    outUavIndex = srvManager->Allocate();
    device->CreateUnorderedAccessView(outResource.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(outUavIndex));

    // SRV ビュー作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture3D.MostDetailedMip = 0;
    srvDesc.Texture3D.MipLevels = 1;

    outSrvIndex = srvManager->Allocate();
    device->CreateShaderResourceView(outResource.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(outSrvIndex));
}

}