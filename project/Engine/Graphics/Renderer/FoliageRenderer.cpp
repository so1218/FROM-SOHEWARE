#include "pch.h"
#include "FoliageRenderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"
#include "EnvironmentManager.h"


namespace FE
{

void FoliageRenderer::Initialize(const RenderEnvironment& env, const std::vector<FoliageTypeConfig>& configs)
{
    if (configs.empty()) return;

    isGenerated_ = false;

    ID3D12Device* device = env.device->GetDevice();
    auto* srvManager = env.srvManager;
    size_t numTypes = configs.size();

    types_.resize(numTypes);

    uint32_t* counterResetMapped = nullptr;
    counterResetUploadBuffer_ = BufferManager::CreateMappedBuffer<uint32_t>(device, 1, &counterResetMapped);
    *counterResetMapped = 0;

    // ★修正: ヒープサイズを動的に計算 (4スロット × 種類数 × フレーム数) に変更
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 4 * static_cast<UINT>(numTypes) * kFrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cullingHeap_));

    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE destCPU = cullingHeap_->GetCPUDescriptorHandleForHeapStart();

    for (int i = 0; i < kFrameCount; ++i)
    {
        cullingDataResource_[i] = BufferManager::CreateMappedConstantBuffer<FoliageCullingData>(device, &mappedCullingData_[i]);

        for (size_t typeIdx = 0; typeIdx < numTypes; ++typeIdx)
        {
            auto& res = types_[typeIdx];
            if (i == 0) {
                res.config = configs[typeIdx]; // 設定の保存
            }

            // 種類ごとの定数バッファを作成
            res.generationDataResource[i] = BufferManager::CreateMappedConstantBuffer<FoliageGenerationData>(device, &res.mappedGenData[i]);
            res.materialResource[i] = BufferManager::CreateMappedConstantBuffer<FoliageMaterialData>(device, &res.mappedMaterial[i]);

            if (i == 0)
            {
                // (前回のコードと同じく、generatedBuffer と appendCounterBuffer を作成する処理)
                res.generatedBuffer = BufferManager::CreateUAVBufferResource(device, sizeof(FoliageInstanceData) * kMaxInstances);
                res.appendCounterBuffer = BufferManager::CreateUAVBufferResource(device, sizeof(uint32_t));
                res.generatedSrvIndex = srvManager->CreateStructuredBufferSRV(res.generatedBuffer.Get(), kMaxInstances, sizeof(FoliageInstanceData));

                res.generatedUavIndex = srvManager->CreateAppendStructuredBufferUAV(
                    res.generatedBuffer.Get(),
                    res.appendCounterBuffer.Get(),
                    kMaxInstances,
                    sizeof(FoliageInstanceData)
                );

                // ★追加: カウンタバッファを ByteAddressBuffer (RawBuffer) として SRV を作成する
                res.counterSrvIndex = srvManager->CreateRawBufferSRV(res.appendCounterBuffer.Get(), sizeof(uint32_t));
            }

            // --- Culling & Draw リソース (フレーム毎) ---
            res.outputInstanceBuffer[i] = BufferManager::CreateUAVBufferResource(device, sizeof(FoliageInstanceData) * kMaxInstances);
            res.indirectArgsBuffer[i] = BufferManager::CreateUAVBufferResource(device, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

            res.indirectArgsUploadBuffer[i] = BufferManager::CreateMappedBuffer<D3D12_DRAW_INDEXED_ARGUMENTS>(device, 1, &res.mappedArgs[i]);
            *res.mappedArgs[i] = {}; // ゼロクリア

            res.outputUavIndex[i] = srvManager->CreateStructuredBufferUAV(res.outputInstanceBuffer[i].Get(), kMaxInstances, sizeof(FoliageInstanceData));
            res.indirectUavIndex[i] = srvManager->CreateRawBufferUAV(res.indirectArgsBuffer[i].Get(), sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

            // ★修正: Cullingヒープへの事前コピー (4つ連続で並べる)
            // 1. t0: InputFoliage
            device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(res.generatedSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            destCPU.ptr += handleSize;

            // ★追加: 2. t1: InstanceCounter
            device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(res.counterSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            destCPU.ptr += handleSize;

            // 3. u0: OutputFoliage
            device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(res.outputUavIndex[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            destCPU.ptr += handleSize;

            // 4. u1: IndirectArgs
            device->CopyDescriptorsSimple(1, destCPU, env.srvManager->GetSRVHandleCPU_ForCopying(res.indirectUavIndex[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            destCPU.ptr += handleSize;
        }
    }

    // 4. Command Signature
    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
    D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc = {};
    cmdSigDesc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
    cmdSigDesc.NumArgumentDescs = 1;
    cmdSigDesc.pArgumentDescs = &argDesc;
    device->CreateCommandSignature(&cmdSigDesc, nullptr, IID_PPV_ARGS(&commandSignature_));
}

void FoliageRenderer::BeginFrame()
{
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void FoliageRenderer::GenerateFoliage(
    const RenderEnvironment& env,
    uint32_t heightMapSrvHandle,
    D3D12_GPU_VIRTUAL_ADDRESS terrainSettingsAddress,
    UINT terrainWidth, UINT terrainDepth)
{
    if (types_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    size_t numTypes = types_.size();

    // ★修正1: 現在の状態を判定（初回は UAV、2回目以降は SRV になっている）
    D3D12_RESOURCE_STATES beforeState = isGenerated_ ?
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE :
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    barriers.reserve(numTypes * 2);

    for (size_t i = 0; i < numTypes; ++i) {
        // ★修正2: 2回目以降なら、頂点生成用バッファ(generatedBuffer)を SRV から UAV に戻す
        if (isGenerated_) {
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                types_[i].generatedBuffer.Get(),
                beforeState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        }

        // カウンタバッファは 0 にリセットするために COPY_DEST へ移行
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
            types_[i].appendCounterBuffer.Get(),
            beforeState, D3D12_RESOURCE_STATE_COPY_DEST));
    }

    if (!barriers.empty()) {
        cmdList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    }

    // --- 0クリア実行 ---
    for (size_t i = 0; i < numTypes; ++i) {
        cmdList->CopyBufferRegion(types_[i].appendCounterBuffer.Get(), 0, counterResetUploadBuffer_.Get(), 0, sizeof(uint32_t));
    }

    // --- クリアが終わったら、カウンタバッファを UAV に戻す ---
    std::vector<D3D12_RESOURCE_BARRIER> counterBarriers2(numTypes);
    for (size_t i = 0; i < numTypes; ++i) {
        counterBarriers2[i] = CD3DX12_RESOURCE_BARRIER::Transition(
            types_[i].appendCounterBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }
    cmdList->ResourceBarrier(static_cast<UINT>(numTypes), counterBarriers2.data());

    // 2. Generation Compute Shader 実行
    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("FoliageGenerationCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("FoliageGenerationCS"));

    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 共通の地形データをバインド
    cmdList->SetComputeRootConstantBufferView(1, terrainSettingsAddress);
    cmdList->SetComputeRootDescriptorTable(2, env.srvManager->GetSRVHandleGPU(heightMapSrvHandle));

    UINT dispatchX = (terrainWidth + 7) / 8;
    UINT dispatchY = (terrainDepth + 7) / 8;

    // ★ 種類ごとに Generation を実行する
    for (size_t i = 0; i < numTypes; ++i)
    {
        auto& res = types_[i];

        // この植物固有の生成データを定数バッファに書き込む
        memcpy(res.mappedGenData[currentFrameIndex_], &res.config.genData, sizeof(FoliageGenerationData));

        // Parameter 0: b0 (個別の生成ルール)
        cmdList->SetComputeRootConstantBufferView(0, res.generationDataResource[currentFrameIndex_]->GetGPUVirtualAddress());

        // ★追加 Parameter 3: t1 (この植物専用の白黒DensityMap)
        cmdList->SetComputeRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(res.config.densityMapSrvHandle));

        // Parameter 4: u0 (この植物専用の出力Appendバッファ)
        cmdList->SetComputeRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(res.generatedUavIndex));

        // Dispatchを実行 (1種類分の植物が生成される)
        cmdList->Dispatch(dispatchX, dispatchY, 1);
    }

    // 3. バリア遷移 (UAV -> SRV)
    std::vector<D3D12_RESOURCE_BARRIER> readBarriers(numTypes * 2);
    for (size_t i = 0; i < numTypes; ++i) {
        readBarriers[i * 2 + 0] = CD3DX12_RESOURCE_BARRIER::Transition(
            types_[i].generatedBuffer.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        readBarriers[i * 2 + 1] = CD3DX12_RESOURCE_BARRIER::Transition(
            types_[i].appendCounterBuffer.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    cmdList->ResourceBarrier(static_cast<UINT>(readBarriers.size()), readBarriers.data());

    // ★追加: 1度生成したことを記録
    isGenerated_ = true;
}

void FoliageRenderer::Draw(
    const RenderEnvironment& env,
    ShadowMap* shadowMap,
    const FoliageCullingData& cullingData)
{
    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12Device* device = env.device->GetDevice();
    size_t numTypes = types_.size();

    // カリングデータの更新
    memcpy(mappedCullingData_[currentFrameIndex_], &cullingData, sizeof(FoliageCullingData));

    // ★修正: ハードコーディングされた meshes 配列は削除し、res.config.mesh を使用します。
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // ==========================================
    // パス 1: GPU カリング (全タイプ回す)
    // ==========================================
    cmdList->SetComputeRootSignature(env.rootSignatureManager->GetRootSignature("FoliageCullingCS"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("FoliageCullingCS"));

    ID3D12DescriptorHeap* cullingHeaps[] = { cullingHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, cullingHeaps);

    for (int typeIdx = 0; typeIdx < numTypes; ++typeIdx)
    {
        auto& res = types_[typeIdx];

        // マテリアル情報を定数バッファに更新
        memcpy(res.mappedMaterial[currentFrameIndex_], &res.config.material, sizeof(FoliageMaterialData));

        // ★修正: メッシュのインデックス数は res.config.mesh から取得
        D3D12_DRAW_INDEXED_ARGUMENTS drawArgs = {};
        drawArgs.IndexCountPerInstance = res.config.mesh->GetIndexCount();
        drawArgs.InstanceCount = 0;
        *res.mappedArgs[currentFrameIndex_] = drawArgs;

        D3D12_RESOURCE_BARRIER resetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            res.indirectArgsBuffer[currentFrameIndex_].Get(),
            D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->ResourceBarrier(1, &resetBarrier);

        cmdList->CopyBufferRegion(
            res.indirectArgsBuffer[currentFrameIndex_].Get(), 0,
            res.indirectArgsUploadBuffer[currentFrameIndex_].Get(), 0,
            sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

        D3D12_RESOURCE_BARRIER csBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(res.outputInstanceBuffer[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(res.indirectArgsBuffer[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        cmdList->ResourceBarrier(2, csBarriers);

        // ディスクリプタのバインド
        // ★修正: 1タイプ・1フレームあたりに使用するディスクリプタ数が [t0, u0, u1] の3個から、[t0, t1, u0, u1] の 4個 に増えます
        UINT slotOffset = (currentFrameIndex_ * numTypes + typeIdx) * 4;
        D3D12_GPU_DESCRIPTOR_HANDLE destGPU = cullingHeap_->GetGPUDescriptorHandleForHeapStart();
        destGPU.ptr += slotOffset * handleSize;

        cmdList->SetComputeRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
        cmdList->SetComputeRootConstantBufferView(1, cullingDataResource_[currentFrameIndex_]->GetGPUVirtualAddress());

        cmdList->SetComputeRootDescriptorTable(2, destGPU); destGPU.ptr += handleSize; // Parameter 2 (t0): InputFoliage

        // ★追加: InstanceCounter (t1) のバインド
        cmdList->SetComputeRootDescriptorTable(3, destGPU); destGPU.ptr += handleSize; // Parameter 3 (t1): InstanceCounter

        // UAVのパラメータ番号をずらします
        cmdList->SetComputeRootDescriptorTable(4, destGPU); destGPU.ptr += handleSize; // Parameter 4 (u0): OutputFoliage
        cmdList->SetComputeRootDescriptorTable(5, destGPU);

        // Dispatch
        UINT maxGroupsX = 1024;
        UINT dispatchX = std::min((UINT)(kMaxInstances + 63) / 64, maxGroupsX);
        UINT dispatchY = ((UINT)kMaxInstances + 65535) / 65536;
        cmdList->Dispatch(dispatchX, dispatchY, 1);
    }

    // ==========================================
    // パス 2: 間接描画 (全タイプ回す)
    // ==========================================
    cmdList->SetPipelineState(env.psoManager->GetPSO("Foliage"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Foliage"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D12DescriptorHeap* mainHeaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, mainHeaps);

    // Parameter 0: b0 (FrameData)
    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    // Parameter 1: b1 (DirectionalLights)
    cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
    // Parameter 2: b4 (GlobalEnvironmentData) -> 仮で globalConstants を入れています。環境用バッファがあれば差し替えてください。
    cmdList->SetGraphicsRootConstantBufferView(2, EnvironmentManager::GetInstance()->GetGlobalEnvironmentResource()->GetGPUVirtualAddress());

    // ShadowMap関連
    if (shadowMap) {
        // Parameter 4: b8 (ShadowData)
        cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
        // Parameter 8: t2 (ShadowMapArray)
        cmdList->SetGraphicsRootDescriptorTable(8, shadowMap->GetSRVHandle());
    }

    for (int typeIdx = 0; typeIdx < numTypes; ++typeIdx)
    {
        auto& res = types_[typeIdx];

        D3D12_RESOURCE_BARRIER drawBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(res.outputInstanceBuffer[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(res.indirectArgsBuffer[currentFrameIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
        };
        cmdList->ResourceBarrier(2, drawBarriers);

        // Parameter 3: b5 (Material) - 植物固有
        cmdList->SetGraphicsRootConstantBufferView(3, res.materialResource[currentFrameIndex_]->GetGPUVirtualAddress());

        // Parameter 5: t10 (InstanceData SRV) - 植物固有
        cmdList->SetGraphicsRootShaderResourceView(5, res.outputInstanceBuffer[currentFrameIndex_]->GetGPUVirtualAddress());

        // Parameter 6: t0 (AlbedoTex) - 植物固有
        cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(res.config.albedoSrvHandle));

        // Parameter 7: t1 (NormalTex) - 植物固有
        cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(res.config.normalSrvHandle));

        // メッシュのバインド
        cmdList->IASetVertexBuffers(0, 1, &res.config.mesh->GetVertexBufferView());
        cmdList->IASetIndexBuffer(&res.config.mesh->GetIndexBufferView());

        // 描画
        cmdList->ExecuteIndirect(
            commandSignature_.Get(), 1,
            res.indirectArgsBuffer[currentFrameIndex_].Get(), 0, nullptr, 0);

        // 次のフレームに備えて Generation 出力バッファの状態を戻す (SRV -> UAV)
        // ★修正: appendCounterBuffer も一緒に UAV に戻す
        D3D12_RESOURCE_BARRIER restoreBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(
                res.generatedBuffer.Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

            CD3DX12_RESOURCE_BARRIER::Transition(
                res.appendCounterBuffer.Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        cmdList->ResourceBarrier(2, restoreBarriers);
    }
}

void FoliageRenderer::UpdateConfigs(const std::vector<FoliageTypeConfig>& configs)
{
    // バッファやヒープはそのまま維持し、設定（テクスチャハンドル等）だけを最新にする
    size_t count = std::min(types_.size(), configs.size());
    for (size_t i = 0; i < count; ++i)
    {
        types_[i].config = configs[i];
    }
}

}