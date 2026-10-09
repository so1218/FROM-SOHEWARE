#include "pch.h"
#include "VolumetricFogPass.h"
#include "Engine.h"
#include "EnvironmentManager.h"
#include "PIXColors.h"

namespace FE
{

void VolumetricFogPass::Initialize(Engine* engine, uint32_t w, uint32_t h, PSOManager* pso)
{
    InitializeBase(engine, w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, true);
    psoManager_ = pso;

    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();

    // 定数バッファの生成と初期化
    constantBuffer_ = BufferManager::CreateMappedConstantBuffer(
        engine->GetGraphicsDevice()->GetDevice(),
        &cbData_
    );

    // PBRベースの光学特性初期値設定
    cbData_->albedo = { 0.8f, 0.8f, 0.8f };
    cbData_->scatteringIntensity = 150.0f;
    cbData_->extinctionScale = 0.2f;
    cbData_->anisotropy = 0.7f;

    cbData_->extinction = 0.005f;
    cbData_->heightDensity = 0.0f;
    cbData_->baseHeight = 0.0f;
    cbData_->heightFalloff = 0.1f;

    cbData_->ambientLight = { 0.0f, 0.0f, 0.0f };
    cbData_->temporalWeight = 0.05f;               
    cbData_->maxDistance = 150.0f;
    cbData_->depthSliceCount = 64.0f;

    cbData_->noiseScale = 0.01f;
    cbData_->noiseDistortion = 0.15f;
    cbData_->windSpeedMultiplier = 1.0f;

    cbData_->coverage = 0.85f;
    cbData_->worleyWeight = 0.9f;
    cbData_->erosion = 0.4f;
    cbData_->noiseFeather = 0.3f;

    cbData_->erosionStrength = 1.0f;
    cbData_->noiseIntensity = 1.0f;

    // 配置式フォグ用CB作成
    volumeConstantBuffer_ = BufferManager::CreateMappedConstantBuffer(
        engine->GetGraphicsDevice()->GetDevice(),
        &volumeCbData_
    );
    memset(volumeCbData_, 0, sizeof(FogVolumeBuffer));
    volumeCbData_->volumeCount = 0;

    // ディスクリプタヒープの作成
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 32;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));
    passHeap_->SetName(L"VolumetricFog_Heap");

    // Froxel用 3Dテクスチャの生成
    CD3DX12_RESOURCE_DESC tex3DDesc = CD3DX12_RESOURCE_DESC::Tex3D(
        DXGI_FORMAT_R16G16B16A16_FLOAT, froxelW, froxelH, froxelD, 1,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    // VoxelInject
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &tex3DDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&voxelInjectRes_));
    voxelInjectRes_->SetName(L"VoxelInjectResource");

    // VoxelInjectFiltered (空間フィルタ後の中間バッファ)
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &tex3DDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&voxelInjectFilteredRes_));
    voxelInjectFilteredRes_->SetName(L"VoxelInjectFilteredResource");

    // VoxelAccumulate (最終積分レイマーチ用ポート)
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &tex3DDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&voxelAccumulateRes_));
    voxelAccumulateRes_->SetName(L"VoxelAccumulateResource");

    // ビュー(SRV/UAV)の登録
    auto* srvManager = engine->GetSRVManager();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    uavDesc.Texture3D.MipSlice = 0;
    uavDesc.Texture3D.FirstWSlice = 0;
    uavDesc.Texture3D.WSize = froxelD;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture3D.MostDetailedMip = 0;
    srvDesc.Texture3D.MipLevels = 1;

    // Inject ビュー作成
    injectUavIndex_ = srvManager->Allocate();
    device->CreateUnorderedAccessView(voxelInjectRes_.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(injectUavIndex_));
    injectSrvIndex_ = srvManager->Allocate();
    device->CreateShaderResourceView(voxelInjectRes_.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(injectSrvIndex_));

    // Filtered ビュー作成
    filteredUavIndex_ = srvManager->Allocate();
    device->CreateUnorderedAccessView(voxelInjectFilteredRes_.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(filteredUavIndex_));
    filteredSrvIndex_ = srvManager->Allocate();
    device->CreateShaderResourceView(voxelInjectFilteredRes_.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(filteredSrvIndex_));

    // Accumulate ビュー作成 (accumUavIndex_ は書き込みに必須)
    accumUavIndex_ = srvManager->Allocate();
    device->CreateUnorderedAccessView(voxelAccumulateRes_.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(accumUavIndex_));
    accumSrvIndex_ = srvManager->Allocate();
    device->CreateShaderResourceView(voxelAccumulateRes_.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(accumSrvIndex_));

    // テンポラル用履歴ピンポンバッファ
    for (int i = 0; i < 2; ++i) {
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &tex3DDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&history3DRes_[i]));
        history3DRes_[i]->SetName(i == 0 ? L"VoxelHistory_0" : L"VoxelHistory_1");

        historyUavIndices_[i] = srvManager->Allocate();
        device->CreateUnorderedAccessView(history3DRes_[i].Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(historyUavIndices_[i]));
        historySrvIndices_[i] = srvManager->Allocate();
        device->CreateShaderResourceView(history3DRes_[i].Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(historySrvIndices_[i]));
    }

    // Resolve パス(2D合成) の最終出力先テクスチャ
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc2D = {};
    uavDesc2D.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc2D.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc2D.Texture2D.MipSlice = 0;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2D = {};
    srvDesc2D.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc2D.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc2D.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc2D.Texture2D.MostDetailedMip = 0;
    srvDesc2D.Texture2D.MipLevels = 1;

    CD3DX12_RESOURCE_DESC resolveTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, w, h, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resolveTexDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resolveOutputRes_));
    resolveOutputRes_->SetName(L"VolumetricFog_ResolveOutput");

    resolveOutputUavIndex_ = srvManager->Allocate();
    device->CreateUnorderedAccessView(resolveOutputRes_.Get(), nullptr, &uavDesc2D, srvManager->GetSRVHandleCPU_ForCopying(resolveOutputUavIndex_));
    resolveOutputSrvIndex_ = srvManager->Allocate();
    device->CreateShaderResourceView(resolveOutputRes_.Get(), &srvDesc2D, srvManager->GetSRVHandleCPU_ForCopying(resolveOutputSrvIndex_));
}

void VolumetricFogPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context, D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    PIXScopedEvent(cmdList, PIXColors::Compute, "Volumetric Fog Pass");

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    uint32_t handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 現在フレームのインデックス管理
    uint32_t currIdx = frameCounter_ % 2;
    uint32_t prevIdx = (frameCounter_ + 1) % 2;

    // 外部リソース（Depth / Shadow）を Compute で読むためのバリア遷移
    D3D12_RESOURCE_BARRIER readBarriers[2] = {};
    readBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(engine_->GetOffscreenDepthResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    readBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(engine_->GetShadowMap()->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(2, readBarriers);

    // 履歴バッファ（前フレームの出力）を SRV（読み込み）へ遷移
    if (frameCounter_ > 0) {
        D3D12_RESOURCE_BARRIER histReadBarrier = CD3DX12_RESOURCE_BARRIER::Transition(history3DRes_[prevIdx].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &histReadBarrier);
    }
    else {
        // 初回フレームのみ COMMON から遷移
        D3D12_RESOURCE_BARRIER histInitBarrier = CD3DX12_RESOURCE_BARRIER::Transition(history3DRes_[prevIdx].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &histInitBarrier);
    }

    // Resolve出力テクスチャの書き込み準備バリア
    D3D12_RESOURCE_BARRIER resolveBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resolveOutputRes_.Get(), (frameCounter_ == 0) ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(1, &resolveBarrier);

    // ヒープのセット
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    D3D12_CPU_DESCRIPTOR_HANDLE destCPU = passHeap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE destGPU = passHeap_->GetGPUDescriptorHandleForHeapStart();

    // スレッドグループ算出用共通変数
    uint32_t dispatch3DX = (froxelW + 7) / 8;
    uint32_t dispatch3DY = (froxelH + 7) / 8;
    uint32_t dispatch3DZ = (froxelD + 3) / 4;

    // ========================================================
    // [1] Injection パス (1点Ditherサンプリング、光と密度の注入)
    // ========================================================
    {
        PIXScopedEvent(cmdList, PIXColors::Compute, "Injection");

        // ----------------------------------------------------
        // ディスクリプタのヒープへのコピー (SRV 3個 + UAV 1個)
        // ----------------------------------------------------
        // t0: SceneDepth
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 0, handleSize), context.GetCPUHandle(context.sceneDepthSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // t1: ShadowMap
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 1, handleSize), engine_->GetShadowMap()->GetSRVHandleCPU(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // t2: NoiseVolume
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 2, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(noise3DData_.srvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // u0: VoxelInject UAV (オフセット 3)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 3, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(injectUavIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VolumetricFogInjectionCS"));
        cmdList->SetPipelineState(psoManager_->GetPSO("VolumetricFogInjectionCS"));

        cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(2, engine_->GetLightManager()->GetPointLightResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(3, engine_->GetLightManager()->GetSpotLightResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(4, volumeConstantBuffer_->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(5, engine_->GetLightManager()->GetShadowDataResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(6, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());

        cmdList->SetComputeRootDescriptorTable(7, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 0, handleSize));
        cmdList->SetComputeRootDescriptorTable(8, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 3, handleSize));

        cmdList->Dispatch(dispatch3DX, dispatch3DY, dispatch3DZ);

        // UAV から SRV（次パスの入力）への同期バリア
        D3D12_RESOURCE_BARRIER injBarrier = CD3DX12_RESOURCE_BARRIER::Transition(voxelInjectRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &injBarrier);
    }


    // ========================================================
    // [2] VoxelSpatialFilter パス (3x3x1 XY空間ノイズのぼかし)
    // ========================================================
    {
        PIXScopedEvent(cmdList, PIXColors::Compute, "Spatial Filter");

        // t0: VoxelInjectCurrent (オフセット 7)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 7, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(injectSrvIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // u0: VoxelInjectFiltered UAV (オフセット 8)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 8, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(filteredUavIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VoxelSpatialFilterCS"));
        cmdList->SetPipelineState(psoManager_->GetPSO("VoxelSpatialFilterCS"));

        cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress()); 
        cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress()); 

        cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 7, handleSize));
        cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 8, handleSize)); 

        cmdList->Dispatch(dispatch3DX, dispatch3DY, dispatch3DZ);

        // UAV から SRV へ同期バリア
        D3D12_RESOURCE_BARRIER filterBarrier = CD3DX12_RESOURCE_BARRIER::Transition(voxelInjectFilteredRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &filterBarrier);
    }


    // ========================================================
    // [3] VoxelTemporalResolve パス (TAA + Clamping)
    // ========================================================
    {
        PIXScopedEvent(cmdList, PIXColors::Compute, "Temporal Resolve");

        // 履歴の書き込み先（currIdx）をあらかじめ UAV 状態に遷移
        D3D12_RESOURCE_BARRIER currHistBarrier = CD3DX12_RESOURCE_BARRIER::Transition(history3DRes_[currIdx].Get(), (frameCounter_ == 0) ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &currHistBarrier);

        // t0: VoxelInjectFiltered (オフセット 9)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 9, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(filteredSrvIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // t1: VoxelHistory [prevIdx] (オフセット 10)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 10, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(historySrvIndices_[prevIdx]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // u0: VoxelTemporalOut [currIdx UAV] (オフセット 11)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 11, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(historyUavIndices_[currIdx]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VoxelTemporalResolveCS"));
        cmdList->SetPipelineState(psoManager_->GetPSO("VoxelTemporalResolveCS"));

        cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress()); 
        cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress());

        cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 9, handleSize));  
        cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 11, handleSize)); 

        cmdList->Dispatch(dispatch3DX, dispatch3DY, dispatch3DZ);

        // TAA完了。書き込まれた履歴[currIdx]を、次パスの積算（レイマーチ）のためにSRVへ遷移
        D3D12_RESOURCE_BARRIER taaBarrier = CD3DX12_RESOURCE_BARRIER::Transition(history3DRes_[currIdx].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &taaBarrier);
    }


    // ========================================================
    // [4] VoxelIntegrate パス (手前から奥へボリュームレイマーチ積算)
    // ========================================================
    {
        PIXScopedEvent(cmdList, PIXColors::Compute, "Integrate");

        // t0: VoxelTemporalOut (上でSRV遷移した history3DRes_[currIdx] を指定 / オフセット 12)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 12, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(historySrvIndices_[currIdx]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // u0: VoxelAccumulate UAV (オフセット 13)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 13, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(accumUavIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VoxelIntegrateCS"));
        cmdList->SetPipelineState(psoManager_->GetPSO("VoxelIntegrateCS"));

        cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress()); 

        cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 12, handleSize)); 
        cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 13, handleSize)); 

        // Z軸方向は1つのスレッドグループ内でループ積分するため、グループサイズは XY のみ
        cmdList->Dispatch(dispatch3DX, dispatch3DY, 1);

        // 積算完了。UAV から SRV へ同期バリア
        D3D12_RESOURCE_BARRIER accBarrier = CD3DX12_RESOURCE_BARRIER::Transition(voxelAccumulateRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &accBarrier);
    }


    // ========================================================
    // [5] VolumetricFogResolve パス (最終2D画面へのアップサンプル合成)
    // ========================================================
    {
        PIXScopedEvent(cmdList, PIXColors::Compute, "Resolve 2D");

        // t0: Depth (オフセット 14)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 14, handleSize), context.GetCPUHandle(context.sceneDepthSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // t1: gVoxelAccumulate (オフセット 15)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 15, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(accumSrvIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // u0: gOutput 2D UAV (オフセット 16)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 16, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(resolveOutputUavIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VolumetricFogResolveCS"));
        cmdList->SetPipelineState(psoManager_->GetPSO("VolumetricFogResolveCS"));

        cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress()); 
        cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress()); 

        cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 14, handleSize)); 
        cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 16, handleSize)); 

        uint32_t clientWidth = Engine::GetClientWidth();
        uint32_t clientHeight = Engine::GetClientHeight();
        cmdList->Dispatch((clientWidth + 7) / 8, (clientHeight + 7) / 8, 1);
    }


    // ====================================================================
    // [6] 後片付け＆次フレームの準備
    // ====================================================================
    // 中間3Dテクスチャを次フレームの計算（UAV）のために初期状態へ戻す
    D3D12_RESOURCE_BARRIER resetBarriers[4] = {};
    resetBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(voxelInjectRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    resetBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(voxelInjectFilteredRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    resetBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(voxelAccumulateRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    resetBarriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(history3DRes_[currIdx].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(4, resetBarriers);

    // 外部リソース（Depth / ShadowMap）をピクセルシェーダー読込（元の状態）に復帰
    readBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    readBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    readBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    readBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(2, readBarriers);

    // 最終出力の2Dテクスチャをグラフィックスパイプライン/ポストエフェクトチェーン用にSRVへ遷移
    auto finalBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resolveOutputRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &finalBarrier);

    // IPostEffect インターフェースへリソースをエクスポート
    this->textureResource_ = resolveOutputRes_;
    this->srvIndex_ = resolveOutputSrvIndex_;

    frameCounter_++;
}

void VolumetricFogPass::BeginFrame()
{
    currentVolumeCount_ = 0;
    if (volumeCbData_)
    {
        volumeCbData_->volumeCount = 0;
    }
}

bool VolumetricFogPass::SubmitFogVolume(const FogVolumeData& volData)
{
    if (!volumeCbData_ || !volData.isVisible || currentVolumeCount_ >= MAX_FOG_VOLUMES)
    {
        return false;
    }

    // Local to World 行列の作成
    Vector3 scaleVec = (volData.type == 0)
        ? Vector3{ volData.scale.x, volData.scale.x, volData.scale.x }
    : volData.scale;

    Matrix4x4 scaleMat = Matrix4x4::MakeScale(scaleVec);
    Matrix4x4 rotMat = Matrix4x4::MakeRotateXYZ({
        Math::ToRadians(volData.rotation.x),
        Math::ToRadians(volData.rotation.y),
        Math::ToRadians(volData.rotation.z)
        });
    Matrix4x4 transMat = Matrix4x4::MakeTranslate(volData.position);
    Matrix4x4 localToWorld = scaleMat * rotMat * transMat;

    // GPU定数バッファのスロットへ直接書き込み
    FogVolume& gpuData = volumeCbData_->volumes[currentVolumeCount_];
    gpuData.worldToLocal = Matrix4x4::Inverse(localToWorld);
    gpuData.type = volData.type;
    gpuData.color = { volData.color.x, volData.color.y, volData.color.z };
    gpuData.density = volData.density;
    gpuData.noiseScale = volData.noiseScale;
    gpuData.noiseIntensity = volData.noiseIntensity;
    gpuData.windDirection = volData.windDirection;
    gpuData.windSpeed = volData.windSpeed;
    gpuData.anisotropy = volData.anisotropy;
    gpuData.blendDistance = volData.blendDistance;
    gpuData.coverage = volData.coverage;
    gpuData.worleyWeight = volData.worleyWeight;
    gpuData.erosion = volData.erosion;
    gpuData.noiseFeather = volData.noiseFeather;
    gpuData.distortionAmount = volData.distortionAmount;
    gpuData.densityOffset = volData.densityOffset;
    gpuData.noiseContrast = volData.noiseContrast;
    gpuData.heightFalloff = volData.heightFalloff;

    currentVolumeCount_++;
    volumeCbData_->volumeCount = currentVolumeCount_;

    return true;
}

}