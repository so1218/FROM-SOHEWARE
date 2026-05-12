#include "pch.h"
#include "VolumetricFogPass.h"
#include "Engine.h"

namespace FE
{

void VolumetricFogPass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso)
{
    // ★重要：アルファチャンネル(透過率)も必要＆HDR値が入るのでFP16を指定
    InitializeBase(engine, w, h, DXGI_FORMAT_R16G16B16A16_FLOAT, true);
    psoManager_ = pso;

    // 設定用CB作成
    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    constantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(VolumetricFogSettings));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    // デフォルトパラメータ設定 (好みに合わせて調整)
    cbData_->density = 0.05f;
    cbData_->scatteringG = 0.7f;   // 0より大きいと光源方向に強く散乱する(Mie散乱)
    cbData_->maxDistance = 150.0f; // 描画限界距離
    cbData_->steps = 32;           // 多いほど綺麗だが重い
    cbData_->intensity = 1.5f;

    cbData_->baseHeight = 0.0f;       // 地面(Y=0)を基準に
    cbData_->heightFalloff = 0.05f;   // 緩やかに上空で薄くする
    cbData_->noiseScale = 0.1f;       // ノイズのスケール
    cbData_->windSpeed = 1.0f;        // 風の強さ
    cbData_->noiseThreshold = 0.0f;
    cbData_->ambientFactor = 0.1f;
    cbData_->fogColor = { 1.0f, 1.0f, 1.0f };

    // ★パス用SRV/UAVヒープ作成（Depth, ShadowMap, OutputUAV の 3つ分）
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 3; // 2 -> 3 に増やす
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));

    passHeap_->SetName(L"VolumetricFog_Heap");
}

void VolumetricFogPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context, D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // --- 1. ディスクリプタの集約コピー ---
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle = passHeap_->GetCPUDescriptorHandleForHeapStart();

    // t0: Depth
    device->CopyDescriptorsSimple(1, destHandle, context.GetCPUHandle(context.sceneDepthSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // t1: ShadowMap
    destHandle.ptr += handleSize;
    device->CopyDescriptorsSimple(1, destHandle, engine_->GetShadowMap()->GetSRVHandleCPU(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // u0: Output (自分自身のテクスチャのUAV)
    // ※srvIndex_ を使って UAV ハンドルを取得（エンジン側の実装に合わせてください）
    destHandle.ptr += handleSize;
    D3D12_CPU_DESCRIPTOR_HANDLE uavHandleCPU = engine_->GetSRVManager()->GetSRVHandleCPU_ForCopying(uavIndex_);
    device->CopyDescriptorsSimple(1, destHandle, uavHandleCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // --- 3. Compute Pipeline 設定 ---
    cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("VolumetricFogCS"));
    cmdList->SetPipelineState(psoManager_->GetPSO("VolumetricFogCS"));

    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // --- 4. ルートパラメータ設定 (CS用RootSignatureの順序に合わせる) ---
    // Param 0: FrameData (b0)
    cmdList->SetComputeRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());

    // Param 1: FogSettings (b2)
    cmdList->SetComputeRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress());

    // Param 2: SRV Table (t0, t1)
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = passHeap_->GetGPUDescriptorHandleForHeapStart();
    cmdList->SetComputeRootDescriptorTable(2, gpuHandle);

    // Param 3: UAV Table (u0)
    gpuHandle.ptr += handleSize * 2;
    cmdList->SetComputeRootDescriptorTable(3, gpuHandle);

    // --- 2. リソースバリア (SRV -> UAV) ---
    // 2. CS用の状態遷移
    PreCompute(cmdList);

    // --- 5. Dispatch 実行 ---
    // スレッドグループサイズ 8x8 に対して、テクスチャ解像度分回す
    UINT dispatchX = (viewport_.Width + 7) / 8;
    UINT dispatchY = (viewport_.Height + 7) / 8;
    cmdList->Dispatch(dispatchX, dispatchY, 1);

    // 4. 元の状態に戻す
    PostCompute(cmdList);
}

}