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

    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    auto* srvManager = engine->GetSRVManager();

    // 1. 設定用Constant Buffer作成
    constantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(FluidSettings));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    // ==========================================
    // Advection（移流）用パラメータ
    // ==========================================
    cbData_->velocityDissipation = 0.98f;  // 速度の減衰率
    cbData_->densityDissipation = 0.99f;  // 密度の減衰率（元の dissipation）
    cbData_->gridScale = 1.0f;   // グリッドの物理サイズ（1マス何メートルか）
    cbData_->paddingFluid1 = 0.0f;   // パディング（未使用）

    // ==========================================
    // Injection（外力）用パラメータ
    // ==========================================
    cbData_->gridMin = { -50.0f, 0.0f, -50.0f }; // 最小座標
    cbData_->interactionRadius = 2.0f;                     // プレイヤーの影響半径（例: 2.0m）

    cbData_->gridMax = { 50.0f, 50.0f,  50.0f }; // 最大座標
    cbData_->injectionStrength = 1.0f;                     // 力の強さの倍率

    cbData_->objectPos = { 0.0f, 0.0f, 0.0f };     // プレイヤーの初期位置
    cbData_->densityAmount = 1.0f;                     // 発生させる霧の量

    cbData_->objectVelocity = { 0.0f, 0.0f, 0.0f };     // プレイヤーの初期速度
    cbData_->paddingFluid2 = 0.0f;

    // 2. ディスクリプタヒープの作成（ダブルバッファリング対応）
    for (int i = 0; i < 2; ++i) {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 128; // 1フレーム96個消費なので128で十分かつ安全
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_[i]));

        std::wstring heapName = L"FluidSimulation_Heap_" + std::to_wstring(i);
        passHeap_[i]->SetName(heapName.c_str());
    }

    // 3. 3Dテクスチャの生成関数（ラムダ式で共通化）
    auto CreateFluidTexture = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& res, DXGI_FORMAT format, LPCWSTR name, uint32_t& uavIdx, uint32_t& srvIdx) {
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex3D(format, width_, height_, depth_, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&res));
        res->SetName(name);

        // UAV
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.WSize = depth_;
        uavIdx = srvManager->Allocate();
        device->CreateUnorderedAccessView(res.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(uavIdx));

        // SRV
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture3D.MipLevels = 1;
        srvIdx = srvManager->Allocate();
        device->CreateShaderResourceView(res.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(srvIdx));
        };

    // 各テクスチャペアを生成
    for (int i = 0; i < 2; ++i) {
        CreateFluidTexture(densityRes_[i], DXGI_FORMAT_R16_FLOAT, L"Fluid_Density", densityUavIndices_[i], densitySrvIndices_[i]);
        CreateFluidTexture(velocityRes_[i], DXGI_FORMAT_R16G16B16A16_FLOAT, L"Fluid_Velocity", velocityUavIndices_[i], velocitySrvIndices_[i]);
        CreateFluidTexture(pressureRes_[i], DXGI_FORMAT_R16_FLOAT, L"Fluid_Pressure", pressureUavIndices_[i], pressureSrvIndices_[i]);
    }
    // Divergenceは1枚
    CreateFluidTexture(divergenceRes_, DXGI_FORMAT_R16_FLOAT, L"Fluid_Divergence", divergenceUavIndex_, divergenceSrvIndex_);
}

void FluidSimulationPass::Execute(ID3D12GraphicsCommandList* cmdList)
{
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // インデックスの更新 (0か1か)
    readIndex_ = frameCounter_ % 2;
    writeIndex_ = (frameCounter_ + 1) % 2;

    // ★修正：フレームに応じて使用するディスクリプタヒープを切り替える（ハザードを完全に防止）
    UINT heapIndex = frameCounter_ % 2;
    ID3D12DescriptorHeap* heaps[] = { passHeap_[heapIndex].Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // ★修正：今フレーム専用のヒープの先頭からポインタを進める
    D3D12_CPU_DESCRIPTOR_HANDLE currentCPU = passHeap_[heapIndex]->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE currentGPU = passHeap_[heapIndex]->GetGPUDescriptorHandleForHeapStart();

    // スレッドグループの計算 (8x8x8スレッドを想定)
    UINT dispatchX = (width_ + 7) / 8;
    UINT dispatchY = (height_ + 7) / 8;
    UINT dispatchZ = (depth_ + 7) / 8;

    // ★ 改善版ヘルパー：複数のSRV/UAVを連続コピーして、1つのDescriptor Tableとしてバインドする
    auto BindDescriptorTable = [&](const std::vector<uint32_t>& srvOrUavIndices, int rootParamIndex) {
        if (srvOrUavIndices.empty()) return;

        // このテーブルの先頭GPUハンドルを記録
        D3D12_GPU_DESCRIPTOR_HANDLE tableStartGPU = currentGPU;

        // 要素数が1つの場合は、同じインデックスを2回並べてサイズ2に揃える
        uint32_t idx0 = srvOrUavIndices[0];
        uint32_t idx1 = (srvOrUavIndices.size() > 1) ? srvOrUavIndices[1] : idx0;

        uint32_t indices[2] = { idx0, idx1 };
        for (uint32_t index : indices) {
            device->CopyDescriptorsSimple(1, currentCPU, engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(index), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            currentCPU.ptr += handleSize;
            currentGPU.ptr += handleSize;
        }

        // ルートパラメータにバインド
        cmdList->SetComputeRootDescriptorTable(rootParamIndex, tableStartGPU);
        };

    // ====================================================================
    // [0] 初回フレームの特殊処理：UAVとして生成されたリソースを前提のSRV状態に遷移
    // ====================================================================
    if (frameCounter_ == 0) {
        D3D12_RESOURCE_BARRIER initBarriers[6] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(pressureRes_[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(pressureRes_[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        cmdList->ResourceBarrier(6, initBarriers);
    }

    // パイプラインの共通バインド (b0: グローバル定数, b1: 流体設定定数)
    cmdList->SetComputeRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("FluidSimulationCS"));
    cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress());


    // ====================================================================
    // [1] Source Injection パス (外部の力・密度を空間に注入)
    // ====================================================================
    {
        // 書き込み先 (writeIndex_) を UAV 状態へ遷移
        D3D12_RESOURCE_BARRIER preBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        cmdList->ResourceBarrier(2, preBarriers);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidInjectionCS"));

        // t0: velocity(read), t1: density(read)
        BindDescriptorTable({ velocitySrvIndices_[readIndex_], densitySrvIndices_[readIndex_] }, 2);
        // u0: velocity(write), u1: density(write)
        BindDescriptorTable({ velocityUavIndices_[writeIndex_], densityUavIndices_[writeIndex_] }, 3);

        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // 次の移流パスで「入力(SRV)」として読むために状態を戻す
        D3D12_RESOURCE_BARRIER postBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        cmdList->ResourceBarrier(2, postBarriers);
    }


    // ====================================================================
    // [2] Advection パス (自己移流：速度と密度を速度場に従って移動させる)
    // ====================================================================
    {
        // 移流先となる readIndex_ 側を UAV 状態へ遷移
        D3D12_RESOURCE_BARRIER preBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        cmdList->ResourceBarrier(2, preBarriers);

        // ★修正：PSOを1つにし、速度と密度を同時に処理する
        cmdList->SetPipelineState(psoManager_->GetPSO("FluidAdvectionCS"));

        // t0: 速度場(前パスの出力), t1: 密度場(前パスの出力)
        BindDescriptorTable({ velocitySrvIndices_[writeIndex_], densitySrvIndices_[writeIndex_] }, 2);
        // u0: 移流先速度, u1: 移流先密度
        BindDescriptorTable({ velocityUavIndices_[readIndex_], densityUavIndices_[readIndex_] }, 3);

        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // 速度だけ次の「発散計算」のために SRV に遷移（密度はUAVのままキープし、これが今フレームの最終結果になる）
        D3D12_RESOURCE_BARRIER postBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &postBarrier);
    }


    // ====================================================================
    // [3] Divergence パス (速度場の「圧縮/発散」を計算)
    // ====================================================================
    {
        cmdList->SetPipelineState(psoManager_->GetPSO("FluidDivergenceCS"));
        BindDescriptorTable({ velocitySrvIndices_[readIndex_] }, 2); // t0: 移流後の速度場
        BindDescriptorTable({ divergenceUavIndex_ }, 3);             // u0: 発散結果目標
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // 次のヤコビ反復で読み込むため、発散バッファを SRV に遷移
        D3D12_RESOURCE_BARRIER postBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            divergenceRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &postBarrier);
    }


    // ====================================================================
    // [4] Jacobi Iteration パス (圧力方程式を20回反復して解く)
    // ====================================================================
    {
        cmdList->SetPipelineState(psoManager_->GetPSO("FluidJacobiCS"));

        const int JACOBI_ITERATIONS = 20;
        for (int i = 0; i < JACOBI_ITERATIONS; ++i)
        {
            int pRead = i % 2;
            int pWrite = (i + 1) % 2;

            // 書き込み先の圧力を UAV 状態へ
            D3D12_RESOURCE_BARRIER preBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
                pressureRes_[pWrite].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            cmdList->ResourceBarrier(1, &preBarrier);

            // t0: 過去の圧力, t1: 速度の発散(Divergence)
            BindDescriptorTable({ pressureSrvIndices_[pRead], divergenceSrvIndex_ }, 2);
            // u0: 新しい圧力の書き込み先
            BindDescriptorTable({ pressureUavIndices_[pWrite] }, 3);
            cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

            // 次の反復（または次のパス）で読めるように SRV 状態へ戻す
            D3D12_RESOURCE_BARRIER postBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
                pressureRes_[pWrite].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            cmdList->ResourceBarrier(1, &postBarrier);
        }

        // 使い終わった Divergence を次フレームのために UAV に戻しておく
        D3D12_RESOURCE_BARRIER resetDivergence = CD3DX12_RESOURCE_BARRIER::Transition(
            divergenceRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &resetDivergence);
    }


    // ====================================================================
    // [5] Projection パス (圧力勾配を速度から引き算し、質量保存を満たす速度場にする)
    // ====================================================================
    {
        uint32_t finalPressureIdx = 0;

        D3D12_RESOURCE_BARRIER preBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &preBarrier);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidSubtractGradientCS"));

        BindDescriptorTable({ velocitySrvIndices_[readIndex_], pressureSrvIndices_[finalPressureIdx] }, 2);
        BindDescriptorTable({ velocityUavIndices_[writeIndex_] }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // ====================================================================
        // ★修正★ 最新の速度 (writeIndex_) を 密度の位置 (readIndex_) に同期させる
        // ====================================================================
        D3D12_RESOURCE_BARRIER copyBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST)
        };
        cmdList->ResourceBarrier(2, copyBarriers);

        // writeIndex_ の内容を readIndex_ へ丸ごと高速コピー
        cmdList->CopyResource(velocityRes_[readIndex_].Get(), velocityRes_[writeIndex_].Get());

        // コピー完了後、両方を NON_PIXEL_SHADER_RESOURCE (SRVとして読める状態) に戻す
        D3D12_RESOURCE_BARRIER postBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        cmdList->ResourceBarrier(2, postBarriers);
    }


    // ====================================================================
    // [6] 最終処理：VolumetricFogPass が密度をテクスチャサンプリングできるようにする
    // ====================================================================
    // 現在の最終的な流体密度は densityRes_[readIndex_] (UAV状態) に残っているため、SRVへ遷移
    D3D12_RESOURCE_BARRIER finalDensityBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        densityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &finalDensityBarrier);

    // これにより、ゲッター GetCurrentDensitySRVIndex() が返す densitySrvIndices_[readIndex_] が
    // VolumetricFogPass のインジェクションCS等から安全に読み込めるようになります。

    // フレームを進める
    frameCounter_++;
}

}