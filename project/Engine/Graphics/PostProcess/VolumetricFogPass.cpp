#include "pch.h"
#include "VolumetricFogPass.h"
#include "Engine.h"

namespace FE
{

void VolumetricFogPass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso)
{
    // アルファチャンネル(透過率)も必要＆HDR値が入るのでFP16を指定
    InitializeBase(engine, w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, true);
    psoManager_ = pso;

    // 設定用CB作成
    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    UINT cbSizeAligned = (sizeof(VolumetricFogSettings) + 255) & ~255;

    // サイズ指定を sizeof() ではなく、アライメント済みの cbSizeAligned に変更
    constantBuffer_ = BufferManager::CreateBufferResource(device, cbSizeAligned);

    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    // 配置式フォグ用CB作成
    volumeConstantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(FogVolumeBuffer));
    volumeConstantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&volumeCbData_));

    // 構造体全体のメモリをゼロクリアしてゴミデータを消す
    memset(volumeCbData_, 0, sizeof(FogVolumeBuffer));

    // count を 0 に明示
    volumeCbData_->volumeCount = 0;

    // PBRベースの光学特性
    cbData_->scatteringColor = { 0.8f, 0.8f, 0.8f }; // 散乱色（1.0以上にして明るさを稼ぐことも可能）
    cbData_->scatteringIntensity = 150.0f;                 // 空間全体のうっすらとした散乱（ゴッドレイのベース）
    cbData_->extinctionScale = 0.2f;                 // 減衰スケール（標準は1.0。光の遮りやすさ）
    cbData_->anisotropy = 0.7f;                      // 位相関数G値（0.7前後で太陽方向に綺麗な筋が出る）

    // --- 密度と高さ ---
    cbData_->globalDensity = 0.005f;                  // 全体的な空間の基本密度
    cbData_->heightDensity = 0.0f;                   // 高さフォグ（雲）の最大密度
    cbData_->baseHeight = 0.0f;                      // 基準の高さ
    cbData_->heightFalloff = 0.1f;                   // 高さによる減衰率

    // --- 環境光とシステム ---
    cbData_->ambientLight = { 0.0f, 0.0f, 0.0f }; // 日陰やフォグ全体に乗る環境光（少し青みを入れると自然）
    cbData_->temporalWeight = 0.05f;                 // TAAの蓄積率
    cbData_->maxDistance = 150.0f;                   // 描画限界
    cbData_->depthSliceCount = 64.0f;                // Z解像度

    // --- ノイズ制御 ---
    cbData_->noiseScale = 0.08f;
    cbData_->noiseDistortion = 0.15f;                // ノイズの歪み
    cbData_->windDirection = { 1.0f, 1.0f, 1.0f };
    cbData_->windSpeed = 0.2f;

    cbData_->coverage = 0.75f;
    cbData_->worleyWeight = 0.8f;
    cbData_->erosion = 0.4f;
    cbData_->noiseFeather = 0.3f;

    cbData_->erosionStrength = 1.0f;
    cbData_->noiseIntensity = 1.0f;

    // パス用SRV/UAVヒープ作成（Depth, ShadowMap, OutputUAV の 3つ分）
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 16; // 10 -> 16 に増やす (流体リソース追加による枯渇を防ぐため)
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));

    passHeap_->SetName(L"VolumetricFog_Heap");

    // ====================================================================
    // Froxel用 3Dテクスチャ（Injection / Accumulation）の生成
    // ====================================================================

    // 1. 3Dテクスチャの定義 (160 x 90 x 64, FP16, UAV許可)
    CD3DX12_RESOURCE_DESC tex3DDesc = CD3DX12_RESOURCE_DESC::Tex3D(
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        froxelW, froxelH, froxelD,
        1, // mipLevels
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    // 2. VoxelInject リソース生成
    device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &tex3DDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // 初期状態はUAV
        nullptr, IID_PPV_ARGS(&voxelInjectRes_));
    voxelInjectRes_->SetName(L"VoxelInjectResource");

    // 3. VoxelAccumulate リソース生成
    device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &tex3DDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // 初期状態はUAV
        nullptr, IID_PPV_ARGS(&voxelAccumulateRes_));
    voxelAccumulateRes_->SetName(L"VoxelAccumulateResource");

    // ====================================================================
    // ディスクリプタヒープ(SRV/UAV)への登録
    // ====================================================================
    auto* srvManager = engine->GetSRVManager();

    // --- UAV (書き込み用ビュー) の定義 ---
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    uavDesc.Texture3D.MipSlice = 0;
    uavDesc.Texture3D.FirstWSlice = 0;
    uavDesc.Texture3D.WSize = froxelD;

    // --- SRV (読み込み用ビュー) の定義 ---
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture3D.MostDetailedMip = 0;
    srvDesc.Texture3D.MipLevels = 1;

    // 4. Inject用のインデックス確保とビュー作成
    injectUavIndex_ = srvManager->Allocate();
    device->CreateUnorderedAccessView(voxelInjectRes_.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(injectUavIndex_));

    injectSrvIndex_ = srvManager->Allocate();
    device->CreateShaderResourceView(voxelInjectRes_.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(injectSrvIndex_)); 

    // 5. Accumulate用のインデックス確保とビュー作成
    accumUavIndex_ = srvManager->Allocate();
    device->CreateUnorderedAccessView(voxelAccumulateRes_.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(accumUavIndex_)); 

    accumSrvIndex_ = srvManager->Allocate();
    device->CreateShaderResourceView(voxelAccumulateRes_.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(accumSrvIndex_));

    // 2D用のUAV/SRV定義
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

    // ====================================================================
    // テンポラル用履歴バッファを "3Dテクスチャ" として2枚作成
    // ====================================================================
    for (int i = 0; i < 2; ++i) {
        device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &tex3DDesc, // tex3DDesc (160x90x64) を使用
            D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(&history3DRes_[i])); // 変数名を history3DRes_ 等に変更推奨

        history3DRes_[i]->SetName(i == 0 ? L"VoxelHistory_0" : L"VoxelHistory_1");

        historyUavIndices_[i] = srvManager->Allocate();
        device->CreateUnorderedAccessView(history3DRes_[i].Get(), nullptr, &uavDesc, // uavDesc (3D用) を使用
            srvManager->GetSRVHandleCPU_ForCopying(historyUavIndices_[i]));

        historySrvIndices_[i] = srvManager->Allocate();
        device->CreateShaderResourceView(history3DRes_[i].Get(), &srvDesc, // srvDesc (3D用) を使用
            srvManager->GetSRVHandleCPU_ForCopying(historySrvIndices_[i]));
    }

    // ====================================================================
    // Resolve パス(2D合成) の最終出力先テクスチャ
    // ====================================================================
    CD3DX12_RESOURCE_DESC resolveTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R16G16B16A16_FLOAT, w, h, 1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resolveTexDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&resolveOutputRes_));
    resolveOutputRes_->SetName(L"VolumetricFog_ResolveOutput");

    resolveOutputUavIndex_ = srvManager->Allocate();
    device->CreateUnorderedAccessView(resolveOutputRes_.Get(), nullptr, &uavDesc2D, // 元の2D用UAVDesc
        srvManager->GetSRVHandleCPU_ForCopying(resolveOutputUavIndex_));

    resolveOutputSrvIndex_ = srvManager->Allocate();
    device->CreateShaderResourceView(resolveOutputRes_.Get(), &srvDesc2D, // 元の2D用SRVDesc
        srvManager->GetSRVHandleCPU_ForCopying(resolveOutputSrvIndex_));
}

void VolumetricFogPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context, D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 1. 現在のフレーム用のインデックスを決定
    uint32_t currIdx = frameCounter_ % 2;
    uint32_t prevIdx = (frameCounter_ + 1) % 2;

    // 1. バリアの整理
    D3D12_RESOURCE_BARRIER temporalBarriers[2] = {};

    // 書き込み先 (currIdx) を UAV に
    D3D12_RESOURCE_STATES currStateBefore = (frameCounter_ == 0) ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    temporalBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(history3DRes_[currIdx].Get(), currStateBefore, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // 読み込み元 (prevIdx) を SRV に (初回フレームのみ COMMON から遷移)
    int barrierCount = 1;
    if (frameCounter_ == 0) {
        temporalBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(history3DRes_[prevIdx].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        barrierCount = 2;
    }
    // Resolve出力用バリア
    D3D12_RESOURCE_BARRIER resolveBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resolveOutputRes_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    if (frameCounter_ > 0) resolveBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    cmdList->ResourceBarrier(barrierCount, temporalBarriers);
    cmdList->ResourceBarrier(1, &resolveBarrier);

    // [0] 前準備：リソース状態の遷移

    D3D12_RESOURCE_BARRIER readBarriers[2] = {};
    readBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->GetOffscreenDepthResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    readBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->GetShadowMap()->GetResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    cmdList->ResourceBarrier(2, readBarriers);

    // ヒープをセット
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // ディスクリプタの先頭ハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE destCPU = passHeap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE destGPU = passHeap_->GetGPUDescriptorHandleForHeapStart();

      // GPUへ送るための配列変換
    std::vector<FogVolume> gpuVolumes; // ローカル変数にするか、メンバ変数の場合はここで clear() する

    for (const auto& volData : editorVolumes_) // editorVolumes_ を回す
    {
        FogVolume gpuData = {};

        // 1. スケール、回転、平行移動から Local To World 行列を作成
        Matrix4x4 scaleMat = Matrix4x4::MakeScale(volData.type == 0 ? Vector3{ volData.scale.x, volData.scale.x, volData.scale.x } : volData.scale);
        Matrix4x4 rotMat = Matrix4x4::MakeRotateXYZ({ Math::ToRadians(volData.rotation.x), Math::ToRadians(volData.rotation.y), Math::ToRadians(volData.rotation.z) });
        Matrix4x4 transMat = Matrix4x4::MakeTranslate(volData.position);

        Matrix4x4 localToWorld = scaleMat * rotMat * transMat;

        // 2. その逆行列 (World To Local) を作ってGPU構造体に入れる
        Matrix4x4 worldToLocal = Matrix4x4::Inverse(localToWorld);

        gpuData.worldToLocal = worldToLocal;
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

        gpuVolumes.push_back(gpuData);
    }

    // 自身の関数を呼んでCBにデータをコピー
    SetFogVolumes(gpuVolumes);

    // [1] Injection パス (3D空間に光と密度を計算)
    // --- ディスクリプタのコピー ---
   // t0: Depth
    device->CopyDescriptorsSimple(1, destCPU, context.GetCPUHandle(context.sceneDepthSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // t1: Shadow
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 1, handleSize), engine_->GetShadowMap()->GetSRVHandleCPU(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // t2: Noise3D
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 2, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(noise3DData_.srvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // t3: Fluid Density
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 3, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(context.fluidDensitySrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // t4: Fluid Velocity
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 4, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(context.fluidVelocitySrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // t5: Fluid UVW（Context等から適切にインデックスを取得）
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 5, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(context.fluidUVWSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // u0: VoxelInject UAV
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 6, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(injectUavIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // --- パイプライン設定 ---
    cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VolumetricFogInjectionCS"));
    cmdList->SetPipelineState(psoManager_->GetPSO("VolumetricFogInjectionCS"));

    // --- ルートパラメータのバインド ---
    cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress()); // b0
    cmdList->SetComputeRootConstantBufferView(1, context.fluidSettingsCBAddress); // b1
    cmdList->SetComputeRootConstantBufferView(2, constantBuffer_->GetGPUVirtualAddress()); // b2
    cmdList->SetComputeRootConstantBufferView(3, engine_->GetLightManager()->GetPointLightResource()->GetGPUVirtualAddress()); // b3
    cmdList->SetComputeRootConstantBufferView(4, engine_->GetLightManager()->GetSpotLightResource()->GetGPUVirtualAddress()); // b4
    cmdList->SetComputeRootConstantBufferView(5, volumeConstantBuffer_->GetGPUVirtualAddress()); // b5

    // t0 ~ t4 のテーブル (前回のルートシグネチャ変更により、t0~t4がひと繋ぎのテーブルになっています)
    cmdList->SetComputeRootDescriptorTable(6, destGPU);

    // u0 のテーブル (※オフセットが4から「5」にズレます！)
    cmdList->SetComputeRootDescriptorTable(7, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 6, handleSize));

    // Dispatch
    UINT injectX = (froxelW + 7) / 8;
    UINT injectY = (froxelH + 7) / 8;
    UINT injectZ = (froxelD + 3) / 4;

    cmdList->Dispatch(injectX, injectY, injectZ);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(voxelInjectRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);

    // [1.5] Temporal Filter パス (3D TAA)
    {
        // t0: Injection Current (オフセット 7)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 7, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(injectSrvIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // t1: Injection History [prev] (オフセット 8)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 8, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(historySrvIndices_[prevIdx]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // u0: Filtered Output [curr] (オフセット 9)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 9, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(historyUavIndices_[currIdx]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VolumetricFogVoxelTemporalFilterCS"));
        cmdList->SetPipelineState(psoManager_->GetPSO("VolumetricFogVoxelTemporalFilterCS"));

        cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress()); // b0
        cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress()); // b2

        cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 7, handleSize)); // t0, t1
        cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 9, handleSize)); // u0

        cmdList->Dispatch(injectX, injectY, injectZ); // Injectionと同じ3DサイズでDispatch

        // 書き込み終わった履歴[curr]をSRVに遷移（Accumulationの入力にするため）
        auto tfBarrier = CD3DX12_RESOURCE_BARRIER::Transition(history3DRes_[currIdx].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &tfBarrier);
    }


    // ========================================================
    // [2] Accumulation パス (手前から奥へ積分)
    // ========================================================
    {
        // FroxelのXY解像度に対するディスパッチサイズ
        UINT accumX = (froxelW + 7) / 8;
        UINT accumY = (froxelH + 7) / 8;

        // t0 を Filter済みの history3DRes_[currIdx] に変更 (オフセット 10)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 10, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(historySrvIndices_[currIdx]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // u0: VoxelAccumulate UAV (オフセット 11)
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 11, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(accumUavIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VolumetricFogAccumulationCS"));
        cmdList->SetPipelineState(psoManager_->GetPSO("VolumetricFogAccumulationCS"));

        cmdList->SetComputeRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());

        cmdList->SetComputeRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 10, handleSize)); // t0
        cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 11, handleSize)); // u0

        cmdList->Dispatch(accumX, accumY, 1);

        auto accBarrier = CD3DX12_RESOURCE_BARRIER::Transition(voxelAccumulateRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &accBarrier);
    }


    // ========================================================
    // [3] Resolve パス (2D画面解像度へ引き伸ばし合成)
    // ========================================================
    {
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 12, handleSize), context.GetCPUHandle(context.sceneDepthSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 13, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(accumSrvIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 14, handleSize), engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(resolveOutputUavIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VolumetricFogResolveCS"));
        cmdList->SetPipelineState(psoManager_->GetPSO("VolumetricFogResolveCS"));

        cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress());

        cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 12, handleSize)); // t0, t1
        cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 14, handleSize)); // u0

        UINT clientWidth = Engine::GetClientWidth();
        UINT clientHeight = Engine::GetClientHeight();
        cmdList->Dispatch((clientWidth + 7) / 8, (clientHeight + 7) / 8, 1);
    }


    // ====================================================================
    // [4] 後片付け：リソース状態を元に戻す
    // ====================================================================
    // 3Dテクスチャを次フレームのために UAV に戻す
    D3D12_RESOURCE_BARRIER resetBarriers[2] = {};
    resetBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(voxelInjectRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    resetBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(voxelAccumulateRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(2, resetBarriers);

    // Depth と ShadowMap を元に戻す
    readBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    readBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    readBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    readBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(2, readBarriers);

    // 3. 後処理：今回の書き込み結果を SRV に戻す（次フレームで履歴として使うため ＆ 後続パスのため）
    auto finalBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resolveOutputRes_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &finalBarrier);

    // IPostEffectの結果として公開
    this->textureResource_ = resolveOutputRes_;        // [currIdx] を削除
    this->srvIndex_ = resolveOutputSrvIndex_;

    frameCounter_++;
}

}