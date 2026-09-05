#include "pch.h"
#include "WorldInteractionPass.h"
#include "Engine.h"
#include "PSOManager.h"
#include "PIXColors.h"

namespace FE
{

WorldInteractionPass::~WorldInteractionPass()
{
    auto* srvManager = engine_->GetSRVManager();
    if (!srvManager) return;

    for (int i = 0; i < 2; ++i)
    {
        if (interactionSrvIndices_[i] != 0) srvManager->FreeSRV(interactionSrvIndices_[i]);
        if (interactionUavIndices_[i] != 0) srvManager->FreeSRV(interactionUavIndices_[i]);
    }

    if (entitySrvIndex_ != 0) srvManager->FreeSRV(entitySrvIndex_);
}

void WorldInteractionPass::Initialize(Engine* engine, PSOManager* psoManager, uint32_t width, uint32_t height)
{
    engine_ = engine;
    psoManager_ = psoManager;
    width_ = width;
    height_ = height;

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    auto* srvManager = engine_->GetSRVManager();

    // 1. 定数バッファ生成
    constantBuffer_ = BufferManager::CreateMappedConstantBuffer<InteractionConstants>(device, &cbData_);

    // デフォルトパラメータ
    constantData_.worldSize = 200.0f;
    constantData_.trailDuration = 3.0f;         // ★ 3.0f = ちょうど3秒間で消滅
    constantData_.terrainHeightScale = 100.0f; // ハイトマップ高度設定
    constantData_.terrainCenter = { -500.0f, -500.0f };
    constantData_.terrainSize = { 1000.0f, 1000.0f };

    // 2. StructuredBuffer (InteractionEntity) の生成
    uint32_t elementSize = sizeof(InteractionEntity);
    uint32_t bufferSize = elementSize * kMaxEntities;

    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&entityBuffer_)
    );

    entityBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedEntityBuffer_));

    // StructuredBuffer用 SRV の作成
    D3D12_SHADER_RESOURCE_VIEW_DESC entitySrvDesc = {};
    entitySrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    entitySrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    entitySrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    entitySrvDesc.Buffer.FirstElement = 0;
    entitySrvDesc.Buffer.NumElements = kMaxEntities;
    entitySrvDesc.Buffer.StructureByteStride = elementSize;
    entitySrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    entitySrvIndex_ = srvManager->Allocate();
    // 1. CopyDescriptorsSimple のソース用 (CPU専用ヒープ)
    device->CreateShaderResourceView(entityBuffer_.Get(), &entitySrvDesc, srvManager->GetSRVHandleCPU_ForCopying(entitySrvIndex_));
    // 2. メインヒープ用 (GPU可視ヒープ / ImGui・他パス参照用)
    device->CreateShaderResourceView(entityBuffer_.Get(), &entitySrvDesc, srvManager->GetSRVHandleCPU(entitySrvIndex_));

    // 3. パス専用ディスクリプタヒープ構築
    for (int i = 0; i < 2; ++i)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 64;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_[i]));
    }

    // 4. Ping-Pong 2D テクスチャ作成用ヘルパー
    auto CreateInteractionTexture = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& res, LPCWSTR name, uint32_t& uavIdx, uint32_t& srvIdx)
        {
            CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R16G16B16A16_FLOAT, width_, height_, 1, 1, 1, 0,
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
            );
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

            device->CreateCommittedResource(
                &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, nullptr,
                IID_PPV_ARGS(&res)
            );
            res->SetName(name);

            // UAV
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

            uavIdx = srvManager->Allocate();
            device->CreateUnorderedAccessView(res.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(uavIdx));
            device->CreateUnorderedAccessView(res.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU(uavIdx));

            // SRV
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels = 1;

            srvIdx = srvManager->Allocate();
            // 1. CopyDescriptorsSimple のソース用
            device->CreateShaderResourceView(res.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(srvIdx));
            // 2. メインヒープ用
            device->CreateShaderResourceView(res.Get(), &srvDesc, srvManager->GetSRVHandleCPU(srvIdx));
        };

    for (int i = 0; i < 2; ++i)
    {
        std::wstring name = L"WorldInteraction_Tex_" + std::to_wstring(i);
        CreateInteractionTexture(interactionRes_[i], name.c_str(), interactionUavIndices_[i], interactionSrvIndices_[i]);
    }
}

void WorldInteractionPass::UpdateEntities(const std::vector<InteractionEntity>& entities)
{
    uint32_t count = static_cast<uint32_t>(std::min(entities.size(), static_cast<size_t>(kMaxEntities)));
    if (count > 0 && mappedEntityBuffer_)
    {
        std::memcpy(mappedEntityBuffer_, entities.data(), sizeof(InteractionEntity) * count);
    }
    constantData_.entityCount = count;
}

void WorldInteractionPass::SetConstants(const InteractionConstants& constants)
{
    auto currentCenter = constantData_.centerWorldPos;
    auto prevCenter = constantData_.prevCenterWorldPos;

    constantData_ = constants;

    // パス内部で管理する座標履歴を復元
    constantData_.centerWorldPos = currentCenter;
    constantData_.prevCenterWorldPos = prevCenter;
}

void WorldInteractionPass::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t terrainHeightMapSrvIndex, const Vector2& centerWorldPos)
{
    if (isFirstFrame_)
    {
        prevCenterWorldPos_ = centerWorldPos;
        isFirstFrame_ = false;
    }

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    uint32_t handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    readIndex_ = frameCounter_ % 2;
    writeIndex_ = (frameCounter_ + 1) % 2;

    // カメラ位置追従用ワープパラメータ設定
    constantData_.centerWorldPos = centerWorldPos;
    constantData_.prevCenterWorldPos = prevCenterWorldPos_;
    *cbData_ = constantData_;// マップドCBVへ転送

    // ヒープ設定
    uint32_t heapIndex = frameCounter_ % 2;
    ID3D12DescriptorHeap* heaps[] = { passHeap_[heapIndex].Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    D3D12_CPU_DESCRIPTOR_HANDLE currentCPU = passHeap_[heapIndex]->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE currentGPU = passHeap_[heapIndex]->GetGPUDescriptorHandleForHeapStart();

    // 1. ルートシグネチャ & 常時定数バッファのバインド
    cmdList->SetComputeRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("WorldInteractionCS"));
    cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress()); // b0
    cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress());                              // b1

    // 2. Descriptor Table (t0: Entities, t1: HeightMap, t2: PrevInteraction)
    D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart = currentGPU;
    uint32_t srvIndices[3] = { entitySrvIndex_, terrainHeightMapSrvIndex, interactionSrvIndices_[readIndex_] };

    for (int i = 0; i < 3; ++i)
    {
        device->CopyDescriptorsSimple(1, currentCPU, engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(srvIndices[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        currentCPU.ptr += handleSize;
        currentGPU.ptr += handleSize;
    }
    cmdList->SetComputeRootDescriptorTable(2, srvTableStart); // RootParam 2

    // 3. Descriptor Table (u0: OutputInteractionMap)
    D3D12_GPU_DESCRIPTOR_HANDLE uavTableStart = currentGPU;
    device->CopyDescriptorsSimple(1, currentCPU, engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(interactionUavIndices_[writeIndex_]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    currentCPU.ptr += handleSize;
    currentGPU.ptr += handleSize;

    cmdList->SetComputeRootDescriptorTable(3, uavTableStart); // RootParam 3

    // 4. 書き込み先リソースを UNORDERED_ACCESS に遷移
    CD3DX12_RESOURCE_BARRIER preBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        interactionRes_[writeIndex_].Get(),
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );
    cmdList->ResourceBarrier(1, &preBarrier);

    // 5. ディスパッチ実行
    cmdList->SetPipelineState(psoManager_->GetPSO("WorldInteractionCS"));
    uint32_t dispatchX = (width_ + 7) / 8;
    uint32_t dispatchY = (height_ + 7) / 8;
    cmdList->Dispatch(dispatchX, dispatchY, 1);

    // 6. 次の描画パス（Grass/Snow等）で読み込めるよう SRV に遷移
    CD3DX12_RESOURCE_BARRIER postBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        interactionRes_[writeIndex_].Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
    );
    cmdList->ResourceBarrier(1, &postBarrier);

    // 書き込みが完了した最新テクスチャの SRV インデックスを記録
    latestSrvIndex_ = interactionSrvIndices_[writeIndex_];

    // フレーム終了処理
    prevCenterWorldPos_ = centerWorldPos;
    frameCounter_++;
}

}