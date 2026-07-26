#include "pch.h"
#include "SSRPass.h"
#include "Engine.h"

namespace FE
{

void SSRPass::Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager)
{
    InitializeBase(engine, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
    psoManager_ = psoManager;

    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    auto* srvManager = engine->GetSRVManager();

    // 定数バッファ
    cbSSR_ = BufferManager::CreateBufferResource(device, (sizeof(SSRSettings) + 255) & ~255);
    cbSSR_->Map(0, nullptr, reinterpret_cast<void**>(&ssrData_));
    *ssrData_ = SSRSettings();
    ssrData_->maxDistance = 50.0f;
    ssrData_->stepSize = 0.1f;
    ssrData_->maxSteps = 128;
    ssrData_->thickness = 0.2f;

    // Hi-Z用の定数バッファ (パスごとにサイズが違うためMip数分作成)
    for (UINT i = 0; i < maxHiZMipLevels_; ++i) {
        cbHiZSettings_[i] = BufferManager::CreateBufferResource(device, 256);
        cbHiZSettings_[i]->Map(0, nullptr, &hiZData_[i]);
    }

    // DescriptorHeap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 64;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));
    passHeap_->SetName(L"SSR_Heap");

    // 中間リソースの共通定義
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // Hi-Z 
    auto hizDesc = texDesc;
    hizDesc.Format = DXGI_FORMAT_R32_FLOAT;
    hizDesc.MipLevels = maxHiZMipLevels_;
    hizDesc.Width = width;
    hizDesc.Height = height;
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &hizDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&hiZRes_));

    // HitResult / Resolve / Spatial
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&hitResultRes_));
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&resolveRes_));
    for (int i = 0; i < 2; ++i) {
        // 最初のバリア遷移が安全に成功するように NON_PIXEL_SHADER_RESOURCE で初期化
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&spatialRes_[i]));
    }

    // Temporal (Ping-Pong 2枚)
    for (int i = 0; i < 2; ++i) {
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&temporalRes_[i]));
    }

    // 全MipをカバーするSRV (Pass 2 SSR Raycast用)
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = maxHiZMipLevels_;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hiZSrvIndex_ = srvManager->Allocate();
    device->CreateShaderResourceView(hiZRes_.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(hiZSrvIndex_));

    // 各MipごとのSRV (Pass 1 ダウンサンプルの入力用)
    for (UINT i = 0; i < maxHiZMipLevels_; ++i) {
        D3D12_SHADER_RESOURCE_VIEW_DESC mipSrvDesc = srvDesc;
        mipSrvDesc.Texture2D.MipLevels = 1;      // 1つのMipだけを読む
        mipSrvDesc.Texture2D.MostDetailedMip = i; // 対象のMip階層

        hiZMipSrvIndices_.push_back(srvManager->Allocate());
        device->CreateShaderResourceView(hiZRes_.Get(), &mipSrvDesc, srvManager->GetSRVHandleCPU_ForCopying(hiZMipSrvIndices_.back()));
    }

    // 各MipごとのUAV
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    for (UINT i = 0; i < maxHiZMipLevels_; ++i) {
        uavDesc.Texture2D.MipSlice = i;
        hiZUavIndices_.push_back(srvManager->Allocate());
        device->CreateUnorderedAccessView(hiZRes_.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(hiZUavIndices_.back()));
    }

    // その他リソースのビュー作成
    auto CreateViews = [&](auto res, uint32_t& uavIdx, uint32_t& srvIdx) {
        srvDesc.Format = uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        srvDesc.Texture2D.MipLevels = 1; uavDesc.Texture2D.MipSlice = 0;
        uavIdx = srvManager->Allocate(); srvIdx = srvManager->Allocate();
        device->CreateUnorderedAccessView(res.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(uavIdx));
        device->CreateShaderResourceView(res.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(srvIdx));
        };

    CreateViews(hitResultRes_, hitResultUavIndex_, hitResultSrvIndex_);
    CreateViews(resolveRes_, resolveUavIndex_, resolveSrvIndex_);
    for (int i = 0; i < 2; ++i) {
        CreateViews(spatialRes_[i], spatialUavIndices_[i], spatialSrvIndices_[i]);
    }
    CreateViews(temporalRes_[0], temporalUavIndices_[0], temporalSrvIndices_[0]);
    CreateViews(temporalRes_[1], temporalUavIndices_[1], temporalSrvIndices_[1]);
}

void SSRPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context, D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    PIXScopedEvent(cmdList, PIX_COLOR(50, 150, 255), "SSR Compute Pipeline");

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto* srvManager = engine_->GetSRVManager();

    uint32_t currIdx = frameCounter_ % 2;
    uint32_t prevIdx = (frameCounter_ + 1) % 2;

    // バリア設定 (GBuffer -> Compute SRV)
    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    // DepthバッファをCompute(SRV)で読めるように遷移
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->GetOffscreenDepthResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    ));
    // resolveRes_ を読み込み用(SRV) から 書き込み用(UAV) に遷移
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        resolveRes_.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    ));

    cmdList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    // ヒープ設定
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);
    D3D12_CPU_DESCRIPTOR_HANDLE destCPU = passHeap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE destGPU = passHeap_->GetGPUDescriptorHandleForHeapStart();
    UINT offset = 0;

    // 共通スレッドサイズ計算関数
    auto DispatchSize = [](UINT size, UINT group) { return (size + group - 1) / group; };

    UINT mipWidth = engine_->GetClientWidth();
    UINT mipHeight = engine_->GetClientHeight();

    // ==========================================
    // 1: Mip 0 に元深度を等倍コピー
    // ==========================================
    cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("HiZ_CopyCS")); 
    cmdList->SetPipelineState(psoManager_->GetPSO("HiZ_CopyCS"));

    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset, handleSize), context.GetCPUHandle(context.sceneDepthSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset + 1, handleSize), srvManager->GetSRVHandleCPU_ForCopying(hiZUavIndices_[0]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cmdList->SetComputeRootDescriptorTable(0, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, offset, handleSize));
    cmdList->SetComputeRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, offset + 1, handleSize));

    cmdList->Dispatch(DispatchSize(mipWidth, 8), DispatchSize(mipHeight, 8), 1);
    offset += 2;

    // ==========================================
    // 1: Mip 1 以降のダウンサンプル
    // ==========================================
    cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("HiZ_DownsampleCS"));
    cmdList->SetPipelineState(psoManager_->GetPSO("HiZ_DownsampleCS"));

    for (UINT i = 1; i < maxHiZMipLevels_; ++i)
    {
        // 前のMip (i-1) を SRV で読めるようにバリア遷移
        auto mipToSrvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            hiZRes_.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            i - 1
        );
        cmdList->ResourceBarrier(1, &mipToSrvBarrier);

        // 定数バッファの更新
        struct HiZSettings { uint32_t inSize[2]; uint32_t outSize[2]; } settings;
        settings.inSize[0] = mipWidth;
        settings.inSize[1] = mipHeight;

        // 解像度を半分にする
        mipWidth = std::max(1u, mipWidth / 2);
        mipHeight = std::max(1u, mipHeight / 2);

        settings.outSize[0] = mipWidth;
        settings.outSize[1] = mipHeight;
        memcpy(hiZData_[i], &settings, sizeof(HiZSettings));

        // 入力SRVは確実に1つ上のMip
        D3D12_CPU_DESCRIPTOR_HANDLE inputDepth = srvManager->GetSRVHandleCPU_ForCopying(hiZMipSrvIndices_[i - 1]);

        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset, handleSize), inputDepth, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset + 1, handleSize), srvManager->GetSRVHandleCPU_ForCopying(hiZUavIndices_[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        cmdList->SetComputeRootConstantBufferView(0, cbHiZSettings_[i]->GetGPUVirtualAddress());
        cmdList->SetComputeRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, offset, handleSize));
        cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, offset + 1, handleSize));

        cmdList->Dispatch(DispatchSize(mipWidth, 8), DispatchSize(mipHeight, 8), 1);
        offset += 2;
    }

    // ループで遷移されなかった一番最後のMipをSRVへ遷移させ、テクスチャ全体を揃える
    auto lastMipBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        hiZRes_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        maxHiZMipLevels_ - 1
    );
    cmdList->ResourceBarrier(1, &lastMipBarrier);

    // ==========================================
    // 2: SSR Raycast
    // ==========================================
    cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("SSR_RaycastCS"));
    cmdList->SetPipelineState(psoManager_->GetPSO("SSR_RaycastCS"));

    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset, handleSize), context.GetCPUHandle(context.normalSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset + 1, handleSize), srvManager->GetSRVHandleCPU_ForCopying(hiZSrvIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset + 2, handleSize), context.GetCPUHandle(context.materialSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset + 3, handleSize), srvManager->GetSRVHandleCPU_ForCopying(hitResultUavIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cmdList->SetComputeRootConstantBufferView(0, cbSSR_->GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(1, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, offset, handleSize)); 
    cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, offset + 3, handleSize)); 

    cmdList->Dispatch(DispatchSize(engine_->GetClientWidth(), 8), DispatchSize(engine_->GetClientHeight(), 8), 1);
    offset += 4;

    auto hitBarrier = CD3DX12_RESOURCE_BARRIER::Transition(hitResultRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &hitBarrier);

    // ==========================================
    // 3: SSR Resolve
    // ==========================================
    cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("SSR_ResolveCS"));
    cmdList->SetPipelineState(psoManager_->GetPSO("SSR_ResolveCS"));

    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset, handleSize), srvManager->GetSRVHandleCPU_ForCopying(hitResultSrvIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset + 1, handleSize), context.GetCPUHandle(context.sceneColorSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset + 2, handleSize), context.GetCPUHandle(context.normalSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset + 3, handleSize), context.GetCPUHandle(context.sceneDepthSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset + 4, handleSize), context.GetCPUHandle(context.materialSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, offset + 5, handleSize), srvManager->GetSRVHandleCPU_ForCopying(resolveUavIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, offset, handleSize)); 
    cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, offset + 5, handleSize)); 

    cmdList->Dispatch(DispatchSize(engine_->GetClientWidth(), 8), DispatchSize(engine_->GetClientHeight(), 8), 1);
    offset += 6;

    auto resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resolveRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &resBarrier);

    // ==========================================
    // 終了処理とエクスポート
    // ==========================================
    // リソースを元に戻す
    D3D12_RESOURCE_BARRIER resetBarriers[3] = {};
    resetBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(engine_->GetOffscreenDepthResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    resetBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(hiZRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    resetBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(hitResultRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    cmdList->ResourceBarrier(3, resetBarriers);

    // 次のポストエフェクトパスがこの結果を参照できるように
    this->textureResource_ = resolveRes_;
    this->srvIndex_ = resolveSrvIndex_;

}

}