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

        // UVWのディスクリプタを解放
        if (uvwSrvIndices_[i] != 0)      srvManager->FreeSRV(uvwSrvIndices_[i]);
        if (uvwUavIndices_[i] != 0)      srvManager->FreeSRV(uvwUavIndices_[i]);
    }
    if (divergenceSrvIndex_ != 0) srvManager->FreeSRV(divergenceSrvIndex_);
    if (divergenceUavIndex_ != 0) srvManager->FreeSRV(divergenceUavIndex_);

    // Curlテクスチャのディスクリプタを解放
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

    // 1. 設定用Constant Buffer作成
    constantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(FluidSettings));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    // ==========================================
    // Advection（移流）用パラメータ
    // ==========================================
    cbData_->velocityDissipation = 0.98f;  // 速度の減衰率
    cbData_->densityDissipation = 0.99f;  // 密度の減衰率（元の dissipation）
    cbData_->gridScale = 1.0f;   // グリッドの物理サイズ（1マス何メートルか）
    cbData_->vorticityStrength = 0.5f;

    // ==========================================
    // Injection（外力）用パラメータ
    // ==========================================
    cbData_->gridMin = { -50.0f, 0.0f, -50.0f }; // 最小座標
    cbData_->interactionRadius = 0.0f;                     // プレイヤーの影響半径（例: 2.0m）

    cbData_->gridMax = { 50.0f, 50.0f,  50.0f }; // 最大座標
    cbData_->injectionStrength = 1.0f;                     // 力の強さの倍率

    cbData_->objectPos = { 0.0f, 0.0f, 0.0f };     // プレイヤーの初期位置
    cbData_->densityAmount = 1.0f;                     // 発生させる霧の量

    cbData_->objectVelocity = { 0.0f, 0.0f, 0.0f };     // プレイヤーの初期速度

    cbData_->dragStrength = 2.0f;         // プレイヤーの移動速度の何倍で空気を引きずるか
    cbData_->pushStrength = 3.0f;         // プレイヤーが空気を外側に押し退ける力の強さ
    cbData_->uvwRelaxation = 0.15f;

    // 2. ディスクリプタヒープの作成（ダブルバッファリング対応）
    for (int i = 0; i < 2; ++i) {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 256; // 1フレーム96個消費なので128で十分かつ安全
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

        // UVW座標用バッファの生成
        CreateFluidTexture(uvwRes_[i], DXGI_FORMAT_R16G16B16A16_FLOAT, L"Fluid_UVW", uvwUavIndices_[i], uvwSrvIndices_[i]);
    }
    // Divergenceは1枚
    CreateFluidTexture(divergenceRes_, DXGI_FORMAT_R16_FLOAT, L"Fluid_Divergence", divergenceUavIndex_, divergenceSrvIndex_);

    // Curlバッファの生成 (1フレーム内で完結するため1枚でOK)
    CreateFluidTexture(curlRes_, DXGI_FORMAT_R16G16B16A16_FLOAT, L"Fluid_Curl", curlUavIndex_, curlSrvIndex_);
}

void FluidSimulationPass::Execute(ID3D12GraphicsCommandList* cmdList)
{
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // インデックスの更新 (0か1か)
    readIndex_ = frameCounter_ % 2;
    writeIndex_ = (frameCounter_ + 1) % 2;

    // フレームに応じて使用するディスクリプタヒープを切り替える
    UINT heapIndex = frameCounter_ % 2;
    ID3D12DescriptorHeap* heaps[] = { passHeap_[heapIndex].Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    D3D12_CPU_DESCRIPTOR_HANDLE currentCPU = passHeap_[heapIndex]->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE currentGPU = passHeap_[heapIndex]->GetGPUDescriptorHandleForHeapStart();

    // スレッドグループの計算
    UINT dispatchX = (width_ + 7) / 8;
    UINT dispatchY = (height_ + 7) / 8;
    UINT dispatchZ = (depth_ + 7) / 8;

    // ディスクリプタバインド用ヘルパー
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

    // ====================================================================
    // 💡 共通バインドはすべてのDispatchの「大前提」として先頭で行う
    // ====================================================================
    cmdList->SetComputeRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("FluidSimulationCS"));
    cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress());

    float voxelSize = cbData_->gridScale;

    // ====================================================================
    // [0] 初回フレームの特殊処理（最優先）
    // ====================================================================
    if (frameCounter_ == 0)
    {
        // 1. プレイヤー位置から初期のグリッドインデックスを記憶
        previousGridX_ = static_cast<int>(std::floor(cbData_->objectPos.x / voxelSize));
        previousGridY_ = static_cast<int>(std::floor(cbData_->objectPos.y / voxelSize));
        previousGridZ_ = static_cast<int>(std::floor(cbData_->objectPos.z / voxelSize));

        cbData_->voxelDelta = { 0, 0, 0 };

        // 2. パイプラインを初期化用に切り替えて実行
        cmdList->SetPipelineState(psoManager_->GetPSO("FluidInitUVWCS"));
        BindDescriptorTable({ uvwUavIndices_[0], densityUavIndices_[0] }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        BindDescriptorTable({ uvwUavIndices_[1], densityUavIndices_[1] }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // バリア遷移
        D3D12_RESOURCE_BARRIER initBarriers[9] = {
              CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
              CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
              CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
              CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
              CD3DX12_RESOURCE_BARRIER::Transition(pressureRes_[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
              CD3DX12_RESOURCE_BARRIER::Transition(pressureRes_[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
              CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
              CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),

              // Curlテクスチャの初回バリア遷移
              CD3DX12_RESOURCE_BARRIER::Transition(curlRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        cmdList->ResourceBarrier(9, initBarriers);
    }
    // ====================================================================
    // [0.5] 2フレーム目以降：Toroidal グリッドのスナップとデータスクロール処理
    // ====================================================================
    else
    {
        int currentGridX = static_cast<int>(std::floor(cbData_->objectPos.x / voxelSize));
        int currentGridY = static_cast<int>(std::floor(cbData_->objectPos.y / voxelSize));
        int currentGridZ = static_cast<int>(std::floor(cbData_->objectPos.z / voxelSize));

        int deltaX = currentGridX - previousGridX_;
        int deltaY = currentGridY - previousGridY_;
        int deltaZ = currentGridZ - previousGridZ_;

        cbData_->voxelDelta = { (float)deltaX, (float)deltaY, (float)deltaZ };

        // グリッド位置のスナップ更新
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

        // 実際に移動があればスクロールを実行
        if (deltaX != 0 || deltaY != 0 || deltaZ != 0)
        {
            D3D12_RESOURCE_BARRIER scrollPreBarriers[3] = {
                CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            };
            cmdList->ResourceBarrier(3, scrollPreBarriers);

            cmdList->SetPipelineState(psoManager_->GetPSO("FluidScrollAndClearCS"));

            BindDescriptorTable({ velocitySrvIndices_[readIndex_], densitySrvIndices_[readIndex_], uvwSrvIndices_[readIndex_] }, 2);
            BindDescriptorTable({ velocityUavIndices_[writeIndex_], densityUavIndices_[writeIndex_], uvwUavIndices_[writeIndex_] }, 3);

            cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

            D3D12_RESOURCE_BARRIER scrollPostBarriers[3] = {
                CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            };
            cmdList->ResourceBarrier(3, scrollPostBarriers);

            // インデックスの魔法の入れ替え
            std::swap(readIndex_, writeIndex_);

            previousGridX_ = currentGridX;
            previousGridY_ = currentGridY;
            previousGridZ_ = currentGridZ;
        }
    }

    // Source Injection パス (外部の力・密度を空間に注入)
    {
        // 書き込み先 (writeIndex_) を UAV 状態へ遷移
        D3D12_RESOURCE_BARRIER preBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        cmdList->ResourceBarrier(2, preBarriers);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidInjectionCS"));

        // t0: velocity, t1: density, t2: noise3D
        BindDescriptorTable({
            velocitySrvIndices_[readIndex_],
            densitySrvIndices_[readIndex_],
            noise3DData_.srvIndex 
            }, 2);

        // u0: velocity(write), u1: density(write)
        BindDescriptorTable({ velocityUavIndices_[writeIndex_], densityUavIndices_[writeIndex_] }, 3);

        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // 次の移流パスで入力(SRV)として読むために状態を戻す
        D3D12_RESOURCE_BARRIER postBarriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        cmdList->ResourceBarrier(2, postBarriers);
    }


    // Advection パス (自己移流：速度と密度を速度場に従って移動)
    {
        // UVWの書き込み先 (readIndex_) も UAV 状態へ遷移
        D3D12_RESOURCE_BARRIER preBarriers[3] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS) // 追加
        };
        cmdList->ResourceBarrier(3, preBarriers);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidAdvectionCS"));

        BindDescriptorTable({ velocitySrvIndices_[writeIndex_], densitySrvIndices_[writeIndex_], uvwSrvIndices_[writeIndex_] }, 2);

        BindDescriptorTable({ velocityUavIndices_[readIndex_], densityUavIndices_[readIndex_], uvwUavIndices_[readIndex_] }, 3);

        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // 速度は次へ、密度は最終結果、UVWは描画や次フレームへ回すためSRV状態に戻す
        D3D12_RESOURCE_BARRIER postBarriers[3] = {
                CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) 
        };
        cmdList->ResourceBarrier(3, postBarriers);
    }

    // 2-Pass Vorticity パス (渦度閉じ込め)
    {
        // Curlの事前計算パス
        // 新しい書き込み先（curlRes_）を UAV へ
        D3D12_RESOURCE_BARRIER preBarrierCurl = CD3DX12_RESOURCE_BARRIER::Transition(
            curlRes_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &preBarrierCurl);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidComputeCurlCS"));

        // t0: velocity(read)
        BindDescriptorTable({ velocitySrvIndices_[readIndex_] }, 2);
        // u0: curl(write)
        BindDescriptorTable({ curlUavIndex_ }, 3);

        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // 渦力の適用パス
        // 計算が終わった Curl を SRV へ、速度の書き込み先（writeIndex_）を UAV へ
        D3D12_RESOURCE_BARRIER preBarriersApply[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(curlRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        cmdList->ResourceBarrier(2, preBarriersApply);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidApplyVorticityCS")); // 元の名称（中身は新パス2のコードに差し替え）

        // t0: velocity, t1: density, t2: curl をそれぞれ割り当てる
        BindDescriptorTable({ velocitySrvIndices_[readIndex_], densitySrvIndices_[readIndex_], curlSrvIndex_ }, 2);
        // u0: 新しい速度場 (writeIndex_)
        BindDescriptorTable({ velocityUavIndices_[writeIndex_] }, 3);

        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // 速度場を次のパスのために SRV に戻す
        D3D12_RESOURCE_BARRIER postBarrierVelocity = CD3DX12_RESOURCE_BARRIER::Transition(
            velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &postBarrierVelocity);
    }

    // Divergence パス (速度場の圧縮/発散を計算)
    {
        cmdList->SetPipelineState(psoManager_->GetPSO("FluidDivergenceCS"));
        BindDescriptorTable({ velocitySrvIndices_[writeIndex_] }, 2);
        BindDescriptorTable({ divergenceUavIndex_ }, 3);
        cmdList->Dispatch(dispatchX, dispatchY, dispatchZ);

        // 次のヤコビ反復で読み込むため、発散バッファを SRV に遷移
        D3D12_RESOURCE_BARRIER postBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            divergenceRes_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &postBarrier);
    }

    // Jacobi Iteration パス (圧力方程式を20回反復して解く)
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

    // Projection パス (圧力勾配を速度から引き算し、質量保存を満たす速度場にする)
    {
        uint32_t finalPressureIdx = 0; // 反復回数が偶数なら0が最新

        D3D12_RESOURCE_BARRIER preBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &preBarrier);

        cmdList->SetPipelineState(psoManager_->GetPSO("FluidSubtractGradientCS"));

        // 読み込み先: writeIndex_
        BindDescriptorTable({ velocitySrvIndices_[writeIndex_], pressureSrvIndices_[finalPressureIdx] }, 2);
        // 書き込み先: readIndex_
        BindDescriptorTable({ velocityUavIndices_[readIndex_] }, 3);

        cmdList->Dispatch(dispatchX, dispatchY, depth_);

        // 最新の 速度、UVW、密度も 待機側 (writeIndex_) に同期コピーさせる
        D3D12_RESOURCE_BARRIER copyBarriers[6] = {
            // Velocity
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST),

            // UVW
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST),

            // 前段でSRVに戻っているので NON_PIXEL_SHADER_RESOURCE から COPY_SOURCE へ遷移させる
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE), 
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST)
        };
        cmdList->ResourceBarrier(6, copyBarriers);

        // コピーの実行（すべてをダブルバッファ間で完全同期）
        cmdList->CopyResource(velocityRes_[writeIndex_].Get(), velocityRes_[readIndex_].Get());
        cmdList->CopyResource(uvwRes_[writeIndex_].Get(), uvwRes_[readIndex_].Get());
        cmdList->CopyResource(densityRes_[writeIndex_].Get(), densityRes_[readIndex_].Get()); 

        // コピー完了後、すべてを NON_PIXEL_SHADER_RESOURCE (SRV) に戻す
        D3D12_RESOURCE_BARRIER postBarriers[6] = {
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(velocityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),

            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(uvwRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),

            // 密度も安全にSRV化
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[readIndex_].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(densityRes_[writeIndex_].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        cmdList->ResourceBarrier(6, postBarriers);
    }

    // フレームを進める
    frameCounter_++;
}

}