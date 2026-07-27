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

    // SRVManager経由で確保したディスクリプタの解放
    for (int i = 0; i < 2; ++i) {
        if (velocitySrvIndices_[i] != 0) srvManager->FreeSRV(velocitySrvIndices_[i]);
        if (velocityUavIndices_[i] != 0) srvManager->FreeSRV(velocityUavIndices_[i]);
        if (densitySrvIndices_[i] != 0)  srvManager->FreeSRV(densitySrvIndices_[i]);
        if (densityUavIndices_[i] != 0)  srvManager->FreeSRV(densityUavIndices_[i]);
        if (pressureSrvIndices_[i] != 0) srvManager->FreeSRV(pressureSrvIndices_[i]);
        if (pressureUavIndices_[i] != 0) srvManager->FreeSRV(pressureUavIndices_[i]);
        if (uvwSrvIndices_[i] != 0)      srvManager->FreeSRV(uvwSrvIndices_[i]);
        if (uvwUavIndices_[i] != 0)      srvManager->FreeSRV(uvwUavIndices_[i]);
    }

    if (divergenceSrvIndex_ != 0) srvManager->FreeSRV(divergenceSrvIndex_);
    if (divergenceUavIndex_ != 0) srvManager->FreeSRV(divergenceUavIndex_);

    if (curlSrvIndex_ != 0) srvManager->FreeSRV(curlSrvIndex_);
    if (curlUavIndex_ != 0) srvManager->FreeSRV(curlUavIndex_);
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

    // GPUに渡すシミュレーション設定パラメータの初期化
    constantBuffer_ = BufferManager::CreateMappedConstantBuffer<FluidSettings>(
        engine->GetGraphicsDevice()->GetDevice(),
        &cbData_
    );

    // 移流・減衰パラメータ
    cbData_->velocityDissipation = 0.98f;
    cbData_->densityDissipation = 0.99f;
    cbData_->gridScale = 1.0f;
    cbData_->vorticityStrength = 0.5f;

    // 外力・インタラクション設定
    cbData_->gridMin = { -50.0f, 0.0f, -50.0f };
    cbData_->gridMax = { 50.0f, 50.0f,  50.0f };
    cbData_->interactionRadius = 0.0f;
    cbData_->injectionStrength = 1.0f;
    cbData_->objectPos = { 0.0f, 0.0f, 0.0f };
    cbData_->objectVelocity = { 0.0f, 0.0f, 0.0f };
    cbData_->densityAmount = 1.0f;

    // オブジェクトの移動による空気抵抗と押し出し係数
    cbData_->dragStrength = 2.0f;
    cbData_->pushStrength = 3.0f;
    cbData_->uvwRelaxation = 0.15f;

    // パス専用のディスクリプタヒープ構築 (ダブルバッファリング対応)
    // 1フレームあたりの最大消費量を見越し、余裕を持って256個確保
    for (int i = 0; i < 2; ++i) {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 256;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_[i]));

        std::wstring heapName = L"FluidSimulation_Heap_" + std::to_wstring(i);
        passHeap_[i]->SetName(heapName.c_str());
    }

    // 3DテクスチャおよびUAV/SRVの生成用ヘルパー
    // 冗長な初期化コードによる可読性低下を防ぐためラムダ式で共通化
    auto CreateFluidTexture = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& res, DXGI_FORMAT format, LPCWSTR name, uint32_t& uavIdx, uint32_t& srvIdx) {
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex3D(format, width_, height_, depth_, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&res));
        res->SetName(name);

        // ComputeShaderでの書き込み用 (UAV)
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.WSize = depth_;

        uavIdx = srvManager->Allocate();
        device->CreateUnorderedAccessView(res.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(uavIdx));

        // 次パス等での読み取り用 (SRV)
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture3D.MipLevels = 1;

        srvIdx = srvManager->Allocate();
        device->CreateShaderResourceView(res.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(srvIdx));
        };

    // シミュレーションに用いる各種テクスチャバッファの生成
    for (int i = 0; i < 2; ++i) {
        CreateFluidTexture(densityRes_[i], DXGI_FORMAT_R16_FLOAT, L"Fluid_Density", densityUavIndices_[i], densitySrvIndices_[i]);
        CreateFluidTexture(velocityRes_[i], DXGI_FORMAT_R16G16B16A16_FLOAT, L"Fluid_Velocity", velocityUavIndices_[i], velocitySrvIndices_[i]);
        CreateFluidTexture(pressureRes_[i], DXGI_FORMAT_R16_FLOAT, L"Fluid_Pressure", pressureUavIndices_[i], pressureSrvIndices_[i]);
        CreateFluidTexture(uvwRes_[i], DXGI_FORMAT_R16G16B16A16_FLOAT, L"Fluid_UVW", uvwUavIndices_[i], uvwSrvIndices_[i]);
    }

    // 計算過程の中間データ用 (フレーム内で処理が完結するため1枚構成)
    CreateFluidTexture(divergenceRes_, DXGI_FORMAT_R16_FLOAT, L"Fluid_Divergence", divergenceUavIndex_, divergenceSrvIndex_);
    CreateFluidTexture(curlRes_, DXGI_FORMAT_R16G16B16A16_FLOAT, L"Fluid_Curl", curlUavIndex_, curlSrvIndex_);
}

void FluidSimulationPass::Execute(ID3D12GraphicsCommandList* cmdList)
{
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Ping-Pongバッファ用のインデックス算出
    // 前フレームの計算結果(read)を入力とし、最新状態(write)を構築
    readIndex_ = frameCounter_ % 2;
    writeIndex_ = (frameCounter_ + 1) % 2;

    // フレーム単位でディスクリプタヒープを切り替え、GPU実行中の上書きを防止
    UINT heapIndex = frameCounter_ % 2;
    ID3D12DescriptorHeap* heaps[] = { passHeap_[heapIndex].Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    D3D12_CPU_DESCRIPTOR_HANDLE currentCPU = passHeap_[heapIndex]->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE currentGPU = passHeap_[heapIndex]->GetGPUDescriptorHandleForHeapStart();

    // 8x8x8スレッドを1グループとしてディスパッチ数を算出
    UINT dispatchX = (width_ + 7) / 8;
    UINT dispatchY = (height_ + 7) / 8;
    UINT dispatchZ = (depth_ + 7) / 8;

    // RootParameterのインデックスに応じて必要なSRV/UAVをヒープへ連続コピー＆バインドするヘルパー
    // 毎回のバインド処理の冗長化を防ぐ
    auto BindDescriptorTable = [&](const std::vector<uint32_t>& srvOrUavIndices, int rootParamIndex) {
        if (srvOrUavIndices.empty()) return;

        D3D12_GPU_DESCRIPTOR_HANDLE tableStartGPU = currentGPU;
        const uint32_t EXPECTED_SIZE = 3;

        for (uint32_t i = 0; i < EXPECTED_SIZE; ++i) {
            uint32_t index = (i < srvOrUavIndices.size()) ? srvOrUavIndices[i] : srvOrUavIndices.back();
            device->CopyDescriptorsSimple(1, currentCPU, engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(index), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            currentCPU.ptr += handleSize;
            currentGPU.ptr += handleSize;
        }
        cmdList->SetComputeRootDescriptorTable(rootParamIndex, tableStartGPU);
        };

    // 全Computeパス共通のルートシグネチャと定数バッファを設定
    cmdList->SetComputeRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("FluidSimulationCS"));
    cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress());

    float voxelSize = cbData_->gridScale;

    if (frameCounter_ == 0)
    {
        // シミュレーション空間の基準となる初期グリッド座標を確定
        previousGridX_ = static_cast<int>(std::floor(cbData_->objectPos.x / voxelSize));
        previousGridY_ = static_cast<int>(std::floor(cbData_->objectPos.y / voxelSize));
        previousGridZ_ = static_cast<int>(std::floor(cbData_->objectPos.z / voxelSize));
        cbData_->voxelDelta = { 0, 0, 0 };

        // UVW座標系の初期化 (レイマーチング時のサンプリング用)
        cmdList->SetPipelineState(psoManager_->GetPSO("FluidInitUVWCS"));
        BindDescriptorTable({ uvwUavIndices_[0], densityUavIndices_[0] }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        BindDescriptorTable({ uvwUavIndices_[1], densityUavIndices_[1] }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // 以降のシミュレーション計算でSRVとして読み込むため状態遷移
        D3D12_RESOURCE_BARRIER initBarriers[9] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[0].Get(),  D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[1].Get(),  D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(pressureRes_[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(pressureRes_[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[0].Get(),      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[1].Get(),      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(curlRes_.Get(),        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        cmdList->ResourceBarrier(9, initBarriers);
    }
    else
    {
        // プレイヤー等の移動に追従するため、循環グリッドのスクロール量を計算
        int currentGridX = static_cast<int>(std::floor(cbData_->objectPos.x / voxelSize));
        int currentGridY = static_cast<int>(std::floor(cbData_->objectPos.y / voxelSize));
        int currentGridZ = static_cast<int>(std::floor(cbData_->objectPos.z / voxelSize));

        int deltaX = currentGridX - previousGridX_;
        int deltaY = currentGridY - previousGridY_;
        int deltaZ = currentGridZ - previousGridZ_;

        cbData_->voxelDelta = { (float)deltaX, (float)deltaY, (float)deltaZ };

        // シミュレーション空間のAABB更新
        Vector3 snappedCenter = {
            static_cast<float>(currentGridX) * voxelSize,
            static_cast<float>(currentGridY) * voxelSize,
            static_cast<float>(currentGridZ) * voxelSize
        };
        float halfWidth = static_cast<float>(width_) * voxelSize * 0.5f;
        float halfHeight = static_cast<float>(height_) * voxelSize * 0.5f;
        float halfDepth = static_cast<float>(depth_) * voxelSize * 0.5f;

        cbData_->gridMin = { snappedCenter.x - halfWidth, snappedCenter.y - halfHeight, snappedCenter.z - halfDepth };
        cbData_->gridMax = { snappedCenter.x + halfWidth, snappedCenter.y + halfHeight, snappedCenter.z + halfDepth };

        // グリッドを跨ぐ移動が発生した場合のみ、内部データをスクロールさせる
        if (deltaX != 0 || deltaY != 0 || deltaZ != 0)
        {
            D3D12_RESOURCE_BARRIER scrollPreBarriers[3] = {
                CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(),  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[writeIndex_].Get(),      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            };
            cmdList->ResourceBarrier(3, scrollPreBarriers);

            cmdList->SetPipelineState(psoManager_->GetPSO("FluidScrollAndClearCS"));
            BindDescriptorTable({ velocitySrvIndices_[readIndex_], densitySrvIndices_[readIndex_], uvwSrvIndices_[readIndex_] }, 2);
            BindDescriptorTable({ velocityUavIndices_[writeIndex_], densityUavIndices_[writeIndex_], uvwUavIndices_[writeIndex_] }, 3);
            cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

            D3D12_RESOURCE_BARRIER scrollPostBarriers[3] = {
                CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(),  D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[writeIndex_].Get(),      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            };
            cmdList->ResourceBarrier(3, scrollPostBarriers);

            // スクロール結果を最新状態として扱うためインデックスをスワップ
            std::swap(readIndex_, writeIndex_);
            previousGridX_ = currentGridX;
            previousGridY_ = currentGridY;
            previousGridZ_ = currentGridZ;
        }
    }

    // 1. Injection (外力・密度の注入)
    {
        D3D12_RESOURCE_BARRIER preBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(),  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        cmdList->ResourceBarrier(2, preBarriers);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidInjectionCS"));
        BindDescriptorTable({ velocitySrvIndices_[readIndex_], densitySrvIndices_[readIndex_], noise3DData_.srvIndex }, 2);
        BindDescriptorTable({ velocityUavIndices_[writeIndex_], densityUavIndices_[writeIndex_] }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        D3D12_RESOURCE_BARRIER postBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(),  D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        cmdList->ResourceBarrier(2, postBarriers);
    }

    // 2. Advection (自己移流：セミラグランジュ法)
    {
        D3D12_RESOURCE_BARRIER preBarriers[3] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[readIndex_].Get(),  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[readIndex_].Get(),      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        cmdList->ResourceBarrier(3, preBarriers);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidAdvectionCS"));
        BindDescriptorTable({ velocitySrvIndices_[writeIndex_], densitySrvIndices_[writeIndex_], uvwSrvIndices_[writeIndex_] }, 2);
        BindDescriptorTable({ velocityUavIndices_[readIndex_],  densityUavIndices_[readIndex_],  uvwUavIndices_[readIndex_] }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        D3D12_RESOURCE_BARRIER postBarriers[3] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[readIndex_].Get(),      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[readIndex_].Get(),  D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        cmdList->ResourceBarrier(3, postBarriers);
    }

    // 3. Vorticity Confinement (数値減衰で失われる微小な渦成分の補填)
    {
        // Curl(回転)場の計算
        D3D12_RESOURCE_BARRIER preBarrierCurl = CD3DX12_RESOURCE_BARRIER::Transition(
            curlRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &preBarrierCurl);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidComputeCurlCS"));
        BindDescriptorTable({ velocitySrvIndices_[readIndex_] }, 2);
        BindDescriptorTable({ curlUavIndex_ }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // 算出されたCurl場を元に速度場へ渦力を適用
        D3D12_RESOURCE_BARRIER preBarriersApply[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(curlRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        cmdList->ResourceBarrier(2, preBarriersApply);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidApplyVorticityCS"));
        BindDescriptorTable({ velocitySrvIndices_[readIndex_], densitySrvIndices_[readIndex_], curlSrvIndex_ }, 2);
        BindDescriptorTable({ velocityUavIndices_[writeIndex_] }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        D3D12_RESOURCE_BARRIER postBarrierVelocity = CD3DX12_RESOURCE_BARRIER::Transition(
            velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &postBarrierVelocity);
    }

    // 4. Divergence (速度場の発散計算)
    {
        cmdList->SetPipelineState(psoManager_->GetPSO("FluidDivergenceCS"));
        BindDescriptorTable({ velocitySrvIndices_[writeIndex_] }, 2);
        BindDescriptorTable({ divergenceUavIndex_ }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        D3D12_RESOURCE_BARRIER postBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            divergenceRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &postBarrier);
    }

    // 5. Jacobi Iteration (ポアソン方程式を解いて圧力場を計算)
    {
        cmdList->SetPipelineState(psoManager_->GetPSO("FluidJacobiCS"));

        // 収束性と計算負荷のトレードオフから反復回数は20回に設定
        const int JACOBI_ITERATIONS = 20;
        for (int i = 0; i < JACOBI_ITERATIONS; ++i)
        {
            int pRead = i % 2;
            int pWrite = (i + 1) % 2;

            D3D12_RESOURCE_BARRIER preBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
                pressureRes_[pWrite].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            cmdList->ResourceBarrier(1, &preBarrier);

            BindDescriptorTable({ pressureSrvIndices_[pRead], divergenceSrvIndex_ }, 2);
            BindDescriptorTable({ pressureUavIndices_[pWrite] }, 3);
            cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

            D3D12_RESOURCE_BARRIER postBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
                pressureRes_[pWrite].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            cmdList->ResourceBarrier(1, &postBarrier);
        }

        // 発散バッファは次フレームのためにUAV状態にリセット
        D3D12_RESOURCE_BARRIER resetDivergence = CD3DX12_RESOURCE_BARRIER::Transition(
            divergenceRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &resetDivergence);
    }

    // 6. Projection (速度場から圧力勾配を引き、質量保存を満たす発散ゼロの速度場にする)
    {
        // 偶数回反復した場合、最終結果はインデックス0に入る
        uint32_t finalPressureIdx = 0;

        D3D12_RESOURCE_BARRIER preBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &preBarrier);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidSubtractGradientCS"));
        BindDescriptorTable({ velocitySrvIndices_[writeIndex_], pressureSrvIndices_[finalPressureIdx] }, 2);
        BindDescriptorTable({ velocityUavIndices_[readIndex_] }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, depth_); // Z成分はdepth_分を処理

        // ダブルバッファ(read/write)の状態を揃えるための同期コピー前準備
        D3D12_RESOURCE_BARRIER copyBarriers[6] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[readIndex_].Get(),      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[writeIndex_].Get(),     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[readIndex_].Get(),  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST)
        };
        cmdList->ResourceBarrier(6, copyBarriers);

        // 各種物理量バッファを同期
        cmdList->CopyResource(velocityRes_[writeIndex_].Get(), velocityRes_[readIndex_].Get());
        cmdList->CopyResource(uvwRes_[writeIndex_].Get(), uvwRes_[readIndex_].Get());
        cmdList->CopyResource(densityRes_[writeIndex_].Get(), densityRes_[readIndex_].Get());

        // 描画および次フレーム入力として用いるため、すべてSRV状態へ遷移
        D3D12_RESOURCE_BARRIER postBarriers[6] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(),D3D12_RESOURCE_STATE_COPY_DEST,   D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[readIndex_].Get(),      D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[writeIndex_].Get(),     D3D12_RESOURCE_STATE_COPY_DEST,   D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[readIndex_].Get(),  D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_COPY_DEST,   D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        cmdList->ResourceBarrier(6, postBarriers);
    }

    frameCounter_++;
}

}